// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_fec.h"
#include "data/sl_core_data_link.h"

#define LOG_NAME SL_CORE_LINK_FEC_LOG_NAME

int sl_core_link_fec_cw_cntrs_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "cntrs_get");

	sl_core_link_state_get(ldev_num, lgrp_num, link_num, &link_state);

	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_dbg(core_link, LOG_NAME, "incorrect (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		return -ENOLINK;
	}

	return sl_core_hw_fec_cw_cntrs_get(core_link, cw_cntrs);
}

int sl_core_link_fec_lane_cntrs_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "lane_cntrs_get");

	sl_core_link_state_get(ldev_num, lgrp_num, link_num, &link_state);

	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_dbg(core_link, LOG_NAME, "incorrect (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		return -ENOLINK;
	}

	return sl_core_hw_fec_lane_cntrs_get(core_link, lane_cntrs);
}

int sl_core_link_fec_tail_cntrs_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "tail_cntrs_get");

	sl_core_link_state_get(ldev_num, lgrp_num, link_num, &link_state);

	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_dbg(core_link, LOG_NAME, "incorrect (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		return -ENOLINK;
	}

	return sl_core_hw_fec_tail_cntrs_get(core_link, tail_cntrs);
}

int sl_core_link_fec_data_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	struct sl_core_link_fec_cw_cntrs *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntr)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "fec_data_get");

	sl_core_link_state_get(ldev_num, lgrp_num, link_num, &link_state);

	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_dbg(core_link, LOG_NAME, "fec_data_get incorrect state (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		return -ENOLINK;
	}

	return sl_core_hw_fec_data_get(core_link, cw_cntrs, lane_cntrs, tail_cntr);
}

s32 sl_core_link_fec_limit_calc(u8 ldev_num, u8 lgrp_num, u8 link_num, int mant, int exp)
{
	return sl_core_data_link_fec_limit_calc(sl_core_link_get(ldev_num, lgrp_num, link_num), mant, exp);
}
