// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>

#include <linux/hpe/sl/sl_lgrp.h>

#include "sl_core_lgrp.h"
#include "data/sl_core_data_lgrp.h"
#include "hw/sl_core_hw_serdes_lane.h"

#define LOG_NAME SL_CORE_LGRP_LOG_NAME

int sl_core_lgrp_new(u8 ldev_num, u8 lgrp_num)
{
	return sl_core_data_lgrp_new(ldev_num, lgrp_num);
}

void sl_core_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id)
{
	sl_core_data_lgrp_connect_id_set(ldev_num, lgrp_num, connect_id);
}

void sl_core_lgrp_del(u8 ldev_num, u8 lgrp_num)
{
	sl_core_data_lgrp_del(ldev_num, lgrp_num);
}

struct sl_core_lgrp *sl_core_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	return sl_core_data_lgrp_get(ldev_num, lgrp_num);
}

int sl_core_lgrp_hw_attr_set(u8 ldev_num, u8 lgrp_num, struct sl_hw_attr *hw_attr)
{
	int                  rtn;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "hw attr set");

	spin_lock(&(core_lgrp->data_lock));
	if (core_lgrp->is_configuring) {
		sl_core_log_err(core_lgrp, LOG_NAME, "hw attr set - lgrp is configuring");
		spin_unlock(&(core_lgrp->data_lock));
		return -EBADRQC;
	}
	core_lgrp->is_configuring = 1;
	spin_unlock(&(core_lgrp->data_lock));

	rtn = sl_core_data_lgrp_link_config_check(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
			"hw attr set - core_data_lgrp_link_config_check failed [%d]", rtn);
		rtn = -EBADRQC;
		goto out;
	}

	sl_core_data_lgrp_hw_attr_set(core_lgrp, hw_attr);

	rtn = 0;

out:
	spin_lock(&(core_lgrp->data_lock));
	core_lgrp->is_configuring = 0;
	spin_unlock(&(core_lgrp->data_lock));

	return rtn;
}

int sl_core_lgrp_config_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_config *lgrp_config)
{
	int                  rtn;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "config set");

	spin_lock(&(core_lgrp->data_lock));
	if (core_lgrp->is_configuring) {
		sl_core_log_err(core_lgrp, LOG_NAME, "config set - lgrp is configuring");
		spin_unlock(&(core_lgrp->data_lock));
		return -EBADRQC;
	}
	core_lgrp->is_configuring = 1;
	spin_unlock(&(core_lgrp->data_lock));

	rtn = sl_core_data_lgrp_link_config_check(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
			"config set - core_data_lgrp_link_config_check failed [%d]", rtn);
		rtn = -EBADRQC;
		goto out;
	}

	sl_core_data_lgrp_config_set(core_lgrp, lgrp_config);

	rtn = 0;

out:
	spin_lock(&(core_lgrp->data_lock));
	core_lgrp->is_configuring = 0;
	spin_unlock(&(core_lgrp->data_lock));

	return rtn;
}

int sl_core_lgrp_pre1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre1)
{
	return sl_core_hw_serdes_lane_pre1_get(core_lgrp, asic_lane_num, pre1);
}

int sl_core_lgrp_pre2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre2)
{
	return sl_core_hw_serdes_lane_pre2_get(core_lgrp, asic_lane_num, pre2);
}

int sl_core_lgrp_pre3_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre3)
{
	return sl_core_hw_serdes_lane_pre3_get(core_lgrp, asic_lane_num, pre3);
}

int sl_core_lgrp_cursor_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *cursor)
{
	return sl_core_hw_serdes_lane_cursor_get(core_lgrp, asic_lane_num, cursor);
}

int sl_core_lgrp_post1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post1)
{
	return sl_core_hw_serdes_lane_post1_get(core_lgrp, asic_lane_num, post1);
}

int sl_core_lgrp_post2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post2)
{
	return sl_core_hw_serdes_lane_post2_get(core_lgrp, asic_lane_num, post2);
}

int sl_core_lgrp_osr_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *osr)
{
	return sl_core_hw_serdes_lane_osr_get(core_lgrp, asic_lane_num, osr);
}

int sl_core_lgrp_encoding_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *encoding)
{
	return sl_core_hw_serdes_lane_encoding_get(core_lgrp, asic_lane_num, encoding);
}

int sl_core_lgrp_width_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *width)
{
	return sl_core_hw_serdes_lane_width_get(core_lgrp, asic_lane_num, width);
}

int sl_core_lgrp_dfe_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *dfe)
{
	return sl_core_hw_serdes_lane_dfe_get(core_lgrp, asic_lane_num, dfe);
}

int sl_core_lgrp_scramble_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *scramble)
{
	return sl_core_hw_serdes_lane_scramble_get(core_lgrp, asic_lane_num, scramble);
}

int sl_core_lgrp_eye_upper_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_upper)
{
	return sl_core_hw_serdes_eye_upper_get(core_lgrp, asic_lane_num, eye_upper);
}

int sl_core_lgrp_eye_lower_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_lower)
{
	return sl_core_hw_serdes_eye_lower_get(core_lgrp, asic_lane_num, eye_lower);
}

/* Shift media lane data based on the table below,
 *
 * | pgrp num | jack type | Swap Nibble |
 * | -------- | --------- | ----------- |
 * | even     | legacy    | no          |
 * | even     | DD        | yes         |
 * | odd      | legacy    | yes         |
 * | odd      | DD        | no          |
 */
bool sl_core_lgrp_media_lane_data_swap(u8 ldev_num, u8 lgrp_num)
{
	u32                  jack_type;
	bool                 swap;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "media lane data swap");

	if (PLATFORM_NIC) {
		sl_core_log_dbg(core_lgrp, LOG_NAME,
				"media lane data swap - NIC platform, no swap");
		return false;
	}

	jack_type    = core_lgrp->serdes.dt.jack_type;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
			"media lane data swap (jack_type = %u)", jack_type);

	if (jack_type == SL_DT_JACK_TYPE_EXAMAX_LEFT || jack_type == SL_DT_JACK_TYPE_EXAMAX_RIGHT ||
	    jack_type == SL_DT_JACK_TYPE_LOW || jack_type == SL_DT_JACK_TYPE_HIGH) {
		sl_core_log_warn(core_lgrp, LOG_NAME, "media lane data swap (Examax jack - no shift)");
		return false;
	}

	swap = (((core_lgrp->num & 1) && jack_type == SL_DT_JACK_TYPE_LEGACY) ||
		(!(core_lgrp->num & 1) && jack_type == SL_DT_JACK_TYPE_DD));

	sl_core_log_dbg(core_lgrp, LOG_NAME, "media lane data swap (swap = %s)", swap ? "yes" : "no");

	return swap;
}

u32 sl_core_lgrp_jack_part_get(u8 ldev_num, u8 lgrp_num)
{
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "jack part get");

	return core_lgrp->serdes.dt.jack_type;
}

int sl_core_lgrp_tx_lane_is_lol(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_tx_lol)
{
	return sl_core_hw_serdes_tx_lane_is_lol(sl_core_lgrp_get(ldev_num, lgrp_num), asic_lane_num, is_tx_lol);
}

int sl_core_lgrp_rx_lane_is_lol(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_rx_lol)
{
	return sl_core_hw_serdes_rx_lane_is_lol(sl_core_lgrp_get(ldev_num, lgrp_num), asic_lane_num, is_rx_lol);
}

int sl_core_lgrp_tx_lane_is_los(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_tx_los)
{
	return sl_core_hw_serdes_tx_lane_is_los(sl_core_lgrp_get(ldev_num, lgrp_num), asic_lane_num, is_tx_los);
}

int sl_core_lgrp_rx_lane_is_los(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_rx_los)
{
	return sl_core_hw_serdes_rx_lane_is_los(sl_core_lgrp_get(ldev_num, lgrp_num), asic_lane_num, is_rx_los);
}
