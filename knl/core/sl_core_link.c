// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include "sl_kconfig.h"

#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_log.h"
#include "data/sl_core_data_link.h"
#include "data/sl_core_data_mac.h"
#include "data/sl_core_data_llr.h"
#include "hw/sl_core_hw_link.h"

#define LOG_NAME SL_CORE_LINK_LOG_NAME

int sl_core_link_new(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_data_link_new(ldev_num, lgrp_num, link_num);
}

void sl_core_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	sl_core_data_link_del(ldev_num, lgrp_num, link_num);
}

struct sl_core_link *sl_core_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_data_link_get(ldev_num, lgrp_num, link_num);
}

int sl_core_link_up(u8 ldev_num, u8 lgrp_num, u8 link_num,
		    sl_core_link_up_callback_t callback, void *tag)
{
	unsigned long        irq_flags;
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME,
		"up (link = 0x%p, flags = 0x%08X)", core_link, core_link->config.flags);

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		sl_core_log_dbg(core_link, LOG_NAME, "up - already going up");
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		return 0;
	case SL_CORE_LINK_STATE_UP:
		sl_core_log_dbg(core_link, LOG_NAME, "up - already up");
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		return 0;
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		sl_core_log_dbg(core_link, LOG_NAME, "up - going up");
		core_link->link.state = SL_CORE_LINK_STATE_GOING_UP;
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		sl_core_hw_link_up_cmd(core_link, callback, tag);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"up - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		return -EBADRQC;
	}
}

int sl_core_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	unsigned long        irq_flags;
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "down");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
	case SL_CORE_LINK_STATE_CONFIGURING:
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		sl_core_log_dbg(core_link, LOG_NAME, "down - already down");
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		return 0;
	case SL_CORE_LINK_STATE_CANCELING:
	case SL_CORE_LINK_STATE_GOING_DOWN:
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "down - already going down");
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		sl_core_hw_link_down_wait(core_link);
		return 0;
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		sl_core_log_dbg(core_link, LOG_NAME, "down - cancel up");
		core_link->link.state = SL_CORE_LINK_STATE_CANCELING;
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		sl_core_hw_link_up_cancel_cmd(core_link);
		return 0;
	case SL_CORE_LINK_STATE_UP:
	case SL_CORE_LINK_STATE_RECOVERING:
		sl_core_log_dbg(core_link, LOG_NAME, "down - going down");
		core_link->link.state = SL_CORE_LINK_STATE_GOING_DOWN;
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		sl_core_hw_link_down_cmd(core_link);
		return 0;
	default:
		sl_core_log_dbg(core_link, LOG_NAME,
			"down - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		return -EBADRQC;
	}
}

int sl_core_link_reset(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	if (!core_link)
		return 0;

	sl_core_hw_reset_link(core_link);

	return 0;
}

int sl_core_link_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *link_state)
{
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	*link_state = sl_core_data_link_state_get(core_link);

	if (*link_state == SL_CORE_LINK_STATE_INVALID)
		return -EINVAL;

	sl_core_log_dbg(core_link, LOG_NAME, "state get (link_state = %u %s)",
		*link_state, sl_core_link_state_str(*link_state));

	return 0;
}

int sl_core_link_data_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_link_data *link_data)
{
// FIXME: this is a temporary fault position.
//        Need to make a data call to get the real stuff.

	link_data->active_lanes = 0;
	link_data->good_eyes    = 0;
	link_data->not_idle     = 0;
	link_data->status       = 0;
	link_data->status      |= SL_LINK_DATA_STATUS_BIT_HISER;
	link_data->status      |= SL_LINK_DATA_STATUS_BIT_FAULT;
	link_data->status      |= SL_LINK_DATA_STATUS_BIT_LOCAL_FAULT;

	return 0;
}

int sl_core_info_map_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u64 *info_map)
{
	struct sl_core_link *core_link;
	struct sl_core_mac  *core_mac;
	struct sl_core_llr  *core_llr;

	BUILD_BUG_ON(SL_CORE_INFO_MAP_NUM_BITS >= 64);

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	core_mac  = sl_core_mac_get(ldev_num, lgrp_num, link_num);

	*info_map = sl_core_data_link_info_map_get(core_link);
	if (core_mac)
		*info_map |= sl_core_data_mac_info_map_get(core_mac);
	core_llr  = sl_core_llr_get(ldev_num, lgrp_num, link_num);

	*info_map = sl_core_data_link_info_map_get(core_link);
	if (core_llr)
		*info_map |= sl_core_data_llr_info_map_get(core_llr);

	sl_core_log_dbg(core_link, LOG_NAME, "info map get (map = 0x%llX)", *info_map);

	return 0;
}

int sl_core_link_config_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			    struct sl_core_link_config *link_config)
{
	unsigned long        irq_flags;
	struct sl_core_link *core_link;
	u32                  link_state;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "config set");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		core_link->link.state = SL_CORE_LINK_STATE_CONFIGURING;
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		sl_core_data_link_config_set(core_link, link_config);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"config set invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		return -EBADRQC;
	}
}

int sl_core_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_policy *link_policy)
{
	unsigned long        irq_flags;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "policy set");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	core_link->policy = *link_policy;
	spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);

	return 0;
}

bool sl_core_link_is_canceled_or_timed_out(struct sl_core_link *core_link)
{
	bool          is_canceled;
	bool          is_timed_out;
	unsigned long irq_flags;

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	is_canceled  = core_link->link.is_canceled;
	is_timed_out = core_link->link.is_timed_out;
	spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);

	sl_core_log_dbg(core_link, LOG_NAME, "is_canceled = %s, is_timed_out = %s",
		is_canceled ? "true" : "false", is_timed_out ? "true" : "false");

	return (is_canceled || is_timed_out);
}

void sl_core_link_is_canceled_set(struct sl_core_link *core_link)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_link, LOG_NAME, "is_canceled_set");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	core_link->link.is_canceled = true;
	spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
}

void sl_core_link_is_canceled_clr(struct sl_core_link *core_link)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_link, LOG_NAME, "is_canceled_clr");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	core_link->link.is_canceled = false;
	spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
}

void sl_core_link_is_timed_out_set(struct sl_core_link *core_link)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_link, LOG_NAME, "is_timed_out_set");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	core_link->link.is_timed_out = true;
	spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
}

void sl_core_link_is_timed_out_clr(struct sl_core_link *core_link)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_link, LOG_NAME, "is_timed_out_clr");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	core_link->link.is_timed_out = false;
	spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
}

int sl_core_link_speed_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *speed)
{
	*speed = sl_core_data_link_speed_get(sl_core_link_get(ldev_num, lgrp_num, link_num));

	return 0;
}

int sl_core_link_clocking_get(struct sl_core_link *core_link, u16 *clocking)
{
	*clocking = sl_core_data_link_clocking_get(core_link);

	return 0;
}

int sl_core_link_last_down_cause_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				     u32 *down_cause, time64_t *down_time)
{
	sl_core_data_link_last_down_cause_get(sl_core_link_get(ldev_num, lgrp_num, link_num),
		down_cause, down_time);

	return 0;
}

bool sl_core_link_policy_is_keep_serdes_up_set(struct sl_core_link *core_link)
{
	unsigned long irq_flags;
	bool          is_policy_set;

	spin_lock_irqsave(&core_link->serdes.data_lock, irq_flags);
	is_policy_set = (core_link->policy.options & SL_LINK_POLICY_OPT_KEEP_SERDES_UP) != 0;
	spin_unlock_irqrestore(&core_link->serdes.data_lock, irq_flags);

	return is_policy_set;
}

bool sl_core_link_policy_is_use_unsupported_cable_set(struct sl_core_link *core_link)
{
	unsigned long irq_flags;
	bool          is_policy_set;

	spin_lock_irqsave(&core_link->serdes.data_lock, irq_flags);
	is_policy_set = (core_link->policy.options & SL_LINK_POLICY_OPT_USE_UNSUPPORTED_CABLE) != 0;
	spin_unlock_irqrestore(&core_link->serdes.data_lock, irq_flags);

	return is_policy_set;
}
