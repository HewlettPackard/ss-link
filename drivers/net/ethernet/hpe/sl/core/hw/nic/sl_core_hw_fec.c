// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_link_fec.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_fec.h"
#include "test/sl_core_test_fec.h"

#define LOG_NAME SL_CORE_HW_FEC_LOG_NAME

static int sl_core_hw_fec_dmac_xfer(struct sl_core_link *core_link, void *data)
{
	int                   rtn;
	struct sl_core_ldev  *core_ldev;

	sl_core_log_dbg(core_link, LOG_NAME, "dmac_xfer");

	core_ldev = core_link->core_lgrp->core_ldev;

	if (!core_ldev->ops.dmac_xfer) {
		sl_core_log_dbg(core_link, LOG_NAME, "dmac_xfer NULL");
		return 0;
	}

	rtn = core_ldev->ops.dmac_xfer(core_ldev->accessors.dmac, data);
	if (rtn) {
		sl_core_log_err(core_link, LOG_NAME, "op dmac_xfer failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

//TODO: remove function SSHOTPLAT-4971
int sl_core_hw_fec_cw_cntrs_get(struct sl_core_link *core_link, struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	int                                rtn;
	struct sl_core_link_fec_data_cntrs data_cntrs;

	sl_core_log_dbg(core_link, LOG_NAME, "cw_cntrs_get");

// FIXME: investigate doing this differently
#if defined(SL_TEST)
	if (core_link->fec.use_test_cntrs) {
		sl_core_log_warn(core_link, LOG_NAME, "cw_cntrs_get using test fec cntrs");
		return sl_core_test_fec_cw_cntrs_get(core_link->core_lgrp->core_ldev->num,
						     core_link->core_lgrp->num, core_link->num, cw_cntrs);
	}
#endif

	rtn = sl_core_hw_fec_dmac_xfer(core_link, &data_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "cw_cntrs_get dmac_xfer failed [%d]", rtn);
		return rtn;
	}

	*cw_cntrs = data_cntrs.cw_cntrs;

	return 0;
}

//TODO: remove function SSHOTPLAT-4971
int sl_core_hw_fec_lane_cntrs_get(struct sl_core_link *core_link, struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	int                                rtn;
	struct sl_core_link_fec_data_cntrs data_cntrs;

	sl_core_log_dbg(core_link, LOG_NAME, "lane_cntrs_get");

	rtn = sl_core_hw_fec_dmac_xfer(core_link, &data_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "lane_cntrs_get dmac_xfer failed [%d]", rtn);
		return rtn;
	}

	*lane_cntrs = data_cntrs.lane_cntrs;

	return 0;
}

//TODO: remove function SSHOTPLAT-4971
int sl_core_hw_fec_tail_cntrs_get(struct sl_core_link *core_link,
				    struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	int                                rtn;
	struct sl_core_link_fec_data_cntrs data_cntrs;

	sl_core_log_dbg(core_link, LOG_NAME, "tail_cntr_get");

	rtn = sl_core_hw_fec_dmac_xfer(core_link, &data_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "tail_cntr_get dmac_xfer failed [%d]", rtn);
		return rtn;
	}

	*tail_cntrs = data_cntrs.tail_cntrs;

	return 0;
}

int sl_core_hw_fec_data_get(struct sl_core_link *core_link,
			    struct sl_core_link_fec_cw_cntrs *cw_cntrs,
			    struct sl_core_link_fec_lane_cntrs *lane_cntrs,
			    struct sl_core_link_fec_tail_cntrs *tail_cntr)
{
	int                                rtn;
	struct sl_core_link_fec_data_cntrs data_cntrs;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_data_get");

	rtn = sl_core_hw_fec_dmac_xfer(core_link, &data_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_data_get dmac_xfer failed [%d]", rtn);
		return rtn;
	}

	*cw_cntrs    = data_cntrs.cw_cntrs;
	*lane_cntrs  = data_cntrs.lane_cntrs;
	*tail_cntr   = data_cntrs.tail_cntrs;

	return 0;
}
