// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_kconfig.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_link_fec.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_io.h"
#include "sl_core_hw_fec.h"
#include "sl_core_hw_fec_test.h"

#define LOG_NAME SL_CORE_HW_FEC_LOG_NAME

/* FEC Cntrs */
#define SL_CORE_UCW_ADDR(_lgrp_num, _link_num) (R2_PF_PML_BASE(_lgrp_num) + \
	SS2_PORT_PML_STS_EVENT_CNTS_OFFSET(BASE_CNTR_IDX_PCS + ROSEVC_PCS_UNCORRECTED_CW_00_INDEX + _link_num))

#define SL_CORE_CCW_ADDR(_lgrp_num, _link_num) (R2_PF_PML_BASE(_lgrp_num) + \
	SS2_PORT_PML_STS_EVENT_CNTS_OFFSET(BASE_CNTR_IDX_PCS + ROSEVC_PCS_CORRECTED_CW_00_INDEX + _link_num))

#define SL_CORE_GCW_ADDR(_lgrp_num, _link_num) (R2_PF_PML_BASE(_lgrp_num) + \
	SS2_PORT_PML_STS_EVENT_CNTS_OFFSET(BASE_CNTR_IDX_PCS + ROSEVC_PCS_GOOD_CW_00_INDEX + _link_num))

/* FEC LANE Cntrs */
#define SL_CORE_LANE_ADDR(_lgrp_num, _link_num, _lane_num) (R2_PF_PML_BASE(_lgrp_num) +          \
	SS2_PORT_PML_STS_EVENT_CNTS_OFFSET(BASE_CNTR_IDX_PCS + ROSEVC_PCS_FECL_ERRORS_00_INDEX + \
					   ((_link_num * SL_CORE_LINK_FEC_NUM_LANES) + _lane_num)))

/* CCW BIN */
#define SL_CORE_CCW_BIN_ADDR(_lgrp_num, _link_num, _bin_num) (R2_PF_PML_BASE(_lgrp_num) +             \
	SS2_PORT_PML_STS_EVENT_CNTS_OFFSET(BASE_CNTR_IDX_PCS + ROSEVC_PCS_CORRECTED_CW_BIN_00_INDEX + \
					   ((_link_num * SL_CORE_LINK_FEC_NUM_CCW_BINS) + _bin_num)))

int sl_core_hw_fec_cw_cntrs_get(struct sl_core_link *core_link, struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	if (core_link->fec.use_test_cntrs) {
		sl_core_log_warn(core_link, LOG_NAME, "cntrs_get using test fec cntrs");
		return sl_core_hw_test_fec_cw_cntrs_get(core_link, cw_cntrs);
	}

	sl_core_read64(core_link, SL_CORE_UCW_ADDR(core_link->core_lgrp->num, core_link->num), &cw_cntrs->ucw);
	sl_core_read64(core_link, SL_CORE_CCW_ADDR(core_link->core_lgrp->num, core_link->num), &cw_cntrs->ccw);
	sl_core_read64(core_link, SL_CORE_GCW_ADDR(core_link->core_lgrp->num, core_link->num), &cw_cntrs->gcw);

	return 0;
}

int sl_core_hw_fec_lane_cntrs_get(struct sl_core_link *core_link, struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	int i;

	sl_core_log_dbg(core_link, LOG_NAME, "lane_cntrs_get");

	for (i = 0; i < SL_CORE_LINK_FEC_NUM_LANES; ++i)
		sl_core_read64(core_link,
			SL_CORE_LANE_ADDR(core_link->core_lgrp->num, core_link->num, i),
			&lane_cntrs->lanes[i]);

	return 0;
}

int sl_core_hw_fec_tail_cntrs_get(struct sl_core_link *core_link,
				    struct sl_core_link_fec_tail_cntrs *tail_cntr)
{
	int i;

	sl_core_log_dbg(core_link, LOG_NAME, "tail_cntr_get");

	for (i = 0; i < SL_CORE_LINK_FEC_NUM_CCW_BINS; ++i)
		sl_core_read64(core_link,
			SL_CORE_CCW_BIN_ADDR(core_link->core_lgrp->num, core_link->num, i),
			&tail_cntr->ccw_bins[i]);

	return 0;
}

int sl_core_hw_fec_data_get(struct sl_core_link *core_link,
			    struct sl_core_link_fec_cw_cntrs *cw_cntrs,
			    struct sl_core_link_fec_lane_cntrs *lane_cntrs,
			    struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	int rtn;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_data_get");

	rtn = sl_core_hw_fec_cw_cntrs_get(core_link, cw_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_cw_cntrs_get failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_core_hw_fec_lane_cntrs_get(core_link, lane_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_lane_cntrs_get failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_core_hw_fec_tail_cntrs_get(core_link, tail_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_lane_cntrs_get failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
