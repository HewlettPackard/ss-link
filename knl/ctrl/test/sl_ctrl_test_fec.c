// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "test/sl_core_test_fec.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_test_fec.h"

#define LOG_NAME SL_CTRL_TEST_FEC_LOG_NAME

static void sl_ctrl_test_fec_data_store_clr(struct sl_ctrl_link *ctrl_link)
{
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "data_store_clr");

	spin_lock(&ctrl_link->fec_data.lock);
	memset(&ctrl_link->fec_data.curr_ptr->cw_cntrs, 0, sizeof(struct sl_core_link_fec_cw_cntrs));
	memset(&ctrl_link->fec_data.curr_ptr->lane_cntrs, 0, sizeof(struct sl_core_link_fec_lane_cntrs));
	memset(&ctrl_link->fec_data.curr_ptr->tail_cntrs, 0, sizeof(struct sl_core_link_fec_tail_cntrs));
	spin_unlock(&ctrl_link->fec_data.lock);
}

int sl_ctrl_test_fec_cntrs_use_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool use_test_cntrs)
{
	int                                 rtn;
	struct sl_ctrl_link                *ctrl_link;
	struct sl_core_link_fec_cw_cntrs    cw_cntrs;
	struct sl_core_link_fec_lane_cntrs  lane_cntrs;
	struct sl_core_link_fec_tail_cntrs  tail_cntrs;

	ctrl_link = sl_ctrl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctrl_link) {
		sl_ctrl_log_err(ctrl_link, LOG_NAME, "ctrl_link_get failed");
		return -EINVAL;
	}

	sl_core_test_fec_cntrs_use_set(ldev_num, lgrp_num, link_num, use_test_cntrs);
	sl_ctrl_test_fec_data_store_clr(ctrl_link);

	rtn = sl_core_link_fec_data_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
		ctrl_link->ctrl_lgrp->num, ctrl_link->num, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn) {
		sl_ctrl_log_err(ctrl_link, LOG_NAME, "fec_data_get failed [%d]", rtn);
		return rtn;
	}

	sl_ctrl_link_fec_data_store(ctrl_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);

	return 0;
}
