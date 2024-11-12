// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "test/sl_core_test_fec.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"

#include "sl_ctl_test_fec.h"

#define LOG_NAME SL_CTL_TEST_FEC_LOG_NAME

static void sl_ctl_test_fec_data_store_clr(struct sl_ctl_link *ctl_link)
{
	unsigned long       irq_flags;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "data_store_clr");

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	memset(&ctl_link->fec_data.curr_ptr->cw_cntrs, 0, sizeof(struct sl_core_link_fec_cw_cntrs));
	memset(&ctl_link->fec_data.curr_ptr->lane_cntrs, 0, sizeof(struct sl_core_link_fec_lane_cntrs));
	memset(&ctl_link->fec_data.curr_ptr->tail_cntrs, 0, sizeof(struct sl_core_link_fec_tail_cntrs));
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);
}

int sl_ctl_test_fec_cntrs_use_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool use_test_cntrs)
{
	int                                 rtn;
	struct sl_ctl_link                 *ctl_link;
	struct sl_core_link_fec_cw_cntrs    cw_cntrs;
	struct sl_core_link_fec_lane_cntrs  lane_cntrs;
	struct sl_core_link_fec_tail_cntrs  tail_cntrs;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "ctl_link_get failed");
		return -EINVAL;
	}

	sl_core_test_fec_cntrs_use_set(ldev_num, lgrp_num, link_num, use_test_cntrs);
	sl_ctl_test_fec_data_store_clr(ctl_link);

	rtn = sl_core_link_fec_data_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "fec_data_get failed [%d]", rtn);
		return rtn;
	}

	sl_ctl_link_fec_data_store(ctl_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);

	return 0;
}
