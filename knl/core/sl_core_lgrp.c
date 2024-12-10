// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/sl_lgrp.h>

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
	unsigned long        irq_flags;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "hw attr set");

	spin_lock_irqsave(&(core_lgrp->data_lock), irq_flags);
	if (core_lgrp->is_configuring) {
		sl_core_log_err(core_lgrp, LOG_NAME, "hw attr set - lgrp is configuring");
		spin_unlock_irqrestore(&(core_lgrp->data_lock), irq_flags);
		return -EBADRQC;
	}
	core_lgrp->is_configuring = 1;
	spin_unlock_irqrestore(&(core_lgrp->data_lock), irq_flags);

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
	spin_lock_irqsave(&(core_lgrp->data_lock), irq_flags);
	core_lgrp->is_configuring = 0;
	spin_unlock_irqrestore(&(core_lgrp->data_lock), irq_flags);

	return rtn;
}

int sl_core_lgrp_config_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_config *lgrp_config)
{
	int                  rtn;
	unsigned long        irq_flags;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "config set");

	spin_lock_irqsave(&(core_lgrp->data_lock), irq_flags);
	if (core_lgrp->is_configuring) {
		sl_core_log_err(core_lgrp, LOG_NAME, "config set - lgrp is configuring");
		spin_unlock_irqrestore(&(core_lgrp->data_lock), irq_flags);
		return -EBADRQC;
	}
	core_lgrp->is_configuring = 1;
	spin_unlock_irqrestore(&(core_lgrp->data_lock), irq_flags);

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
	spin_lock_irqsave(&(core_lgrp->data_lock), irq_flags);
	core_lgrp->is_configuring = 0;
	spin_unlock_irqrestore(&(core_lgrp->data_lock), irq_flags);

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

const char *sl_core_lgrp_serdes_state_str(u8 state)
{
	switch (state) {
	case SL_CORE_LGRP_SERDES_STATE_UNKNOWN:
		return "unknown";
	case SL_CORE_LGRP_SERDES_STATE_INIT:
		return "init";
	case SL_CORE_LGRP_SERDES_STATE_READY:
		return "ready";
	case SL_CORE_LGRP_SERDES_STATE_ERROR:
		return "error";
	default:
		return "invalid";
	}
}
