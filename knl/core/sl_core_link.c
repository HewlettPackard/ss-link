// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

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
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME,
		"up (link = 0x%p, flags = 0x%08X)", core_link, core_link->config.flags);

	if (!sl_core_ldev_serdes_is_ready(core_link->core_lgrp->core_ldev)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up serdes isn't ready");
		return -EIO;
	}

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		sl_core_log_dbg(core_link, LOG_NAME, "up - already going up");
		spin_unlock(&core_link->link.data_lock);
		return 0;
	case SL_CORE_LINK_STATE_UP:
		sl_core_log_dbg(core_link, LOG_NAME, "up - already up");
		spin_unlock(&core_link->link.data_lock);
		return 0;
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		sl_core_log_dbg(core_link, LOG_NAME, "up - going up");
		core_link->link.state = is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE) ?
			SL_CORE_LINK_STATE_AN : SL_CORE_LINK_STATE_GOING_UP;
		spin_unlock(&core_link->link.data_lock);
		sl_core_hw_link_up_cmd(core_link, callback, tag);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"up - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return -EBADRQC;
	}
}

int sl_core_link_up_fail(struct sl_core_link *core_link)
{
	u32                  link_state;

	sl_core_log_dbg(core_link, LOG_NAME, "up fail");

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_CANCELING:
	case SL_CORE_LINK_STATE_GOING_DOWN:
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "up fail - already going down");
		spin_unlock(&core_link->link.data_lock);
		return 0;
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		sl_core_log_dbg(core_link, LOG_NAME, "up fail - going down");
		core_link->link.state = SL_CORE_LINK_STATE_GOING_DOWN;
		spin_unlock(&core_link->link.data_lock);
		sl_core_work_link_queue(core_link, SL_CORE_WORK_LINK_UP_FAIL);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"up fail - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return -EBADRQC;
	}
}

int sl_core_link_cancel(u8 ldev_num, u8 lgrp_num, u8 link_num,
		      sl_core_link_down_callback_t callback, void *tag)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "cancel");

	if (!callback) {
		sl_core_log_err(core_link, LOG_NAME, "cancel - NULL callback");
		return -EINVAL;
	}

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_CANCELING:
	case SL_CORE_LINK_STATE_GOING_DOWN:
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "cancel - already going down");
		spin_unlock(&core_link->link.data_lock);
		return 0;
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		sl_core_log_dbg(core_link, LOG_NAME, "canceling");
		core_link->link.tags.down      = tag;
		core_link->link.callbacks.down = callback;
		core_link->link.state = SL_CORE_LINK_STATE_CANCELING;
		if (!queue_work(core_link->core_lgrp->core_ldev->workqueue,
			&(core_link->work[SL_CORE_WORK_LINK_UP_CANCEL])))
			sl_core_log_warn(core_link, LOG_NAME, "already queued (work_num = %u)",
				SL_CORE_WORK_LINK_UP_CANCEL);
		spin_unlock(&core_link->link.data_lock);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"down - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return -EBADRQC;
	}
}

int sl_core_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num,
		      sl_core_link_down_callback_t callback, void *tag)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "down");

	if (!callback) {
		sl_core_log_err(core_link, LOG_NAME, "down - NULL callback");
		return -EINVAL;
	}

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_CANCELING:
	case SL_CORE_LINK_STATE_GOING_DOWN:
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "down - already going down");
		spin_unlock(&core_link->link.data_lock);
		return 0;
	case SL_CORE_LINK_STATE_UP:
	case SL_CORE_LINK_STATE_RECOVERING:
		sl_core_log_dbg(core_link, LOG_NAME, "down - going down");
		core_link->link.tags.down      = tag;
		core_link->link.callbacks.down = callback;
		core_link->link.state = SL_CORE_LINK_STATE_GOING_DOWN;
		spin_unlock(&core_link->link.data_lock);
		sl_core_work_link_queue(core_link, SL_CORE_WORK_LINK_DOWN);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"down - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
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
	struct sl_core_link *core_link;
	u32                  link_state;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "config set");

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		core_link->link.state = SL_CORE_LINK_STATE_CONFIGURING;
		spin_unlock(&core_link->link.data_lock);
		sl_core_data_link_config_set(core_link, link_config);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"config set invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return -EBADRQC;
	}
}

int sl_core_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_policy *link_policy)
{
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "policy set");

	spin_lock(&core_link->link.data_lock);
	core_link->policy = *link_policy;
	spin_unlock(&core_link->link.data_lock);

	return 0;
}

int sl_core_link_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_link_caps *link_caps)
{
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "caps get");

	*link_caps = core_link->core_lgrp->link_caps[core_link->num];

	return 0;
}

bool sl_core_link_is_canceled_or_timed_out(struct sl_core_link *core_link)
{
	u32 state;
	bool is_canceled;
	bool is_timed_out;

	state = sl_core_data_link_state_get(core_link);

	is_canceled = (state == SL_CORE_LINK_STATE_CANCELING);
	is_timed_out = (state == SL_CORE_LINK_STATE_TIMEOUT);

	sl_core_log_dbg(core_link, LOG_NAME, "is_canceled = %s, is_timed_out = %s",
		is_canceled ? "true" : "false", is_timed_out ? "true" : "false");

	return (is_canceled || is_timed_out);
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

void sl_core_link_last_up_fail_cause_map_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	u64 *up_fail_cause_map, time64_t *up_fail_time)
{
	sl_core_data_link_last_up_fail_info_get(sl_core_link_get(ldev_num, lgrp_num, link_num),
		up_fail_cause_map, up_fail_time);
}

void sl_core_link_last_down_cause_map_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
					       u64 down_cause_map)
{
	sl_core_data_link_last_down_cause_map_set(sl_core_link_get(ldev_num, lgrp_num, link_num),
						  down_cause_map);
}

void sl_core_link_last_down_cause_map_info_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
					       u64 *down_cause_map, time64_t *down_time)
{
	sl_core_data_link_last_down_cause_map_info_get(sl_core_link_get(ldev_num, lgrp_num, link_num),
						   down_cause_map, down_time);
}

void sl_core_link_last_up_fail_cause_map_set(u8 ldev_num, u8 lgrp_num, u8 link_num, u64 up_fail_cause_map)
{
	sl_core_data_link_last_up_fail_cause_map_set(sl_core_link_get(ldev_num, lgrp_num, link_num),
					      up_fail_cause_map);
}

void sl_core_link_ucw_warn_limit_crossed_get(u8 ldev_num, u8 lgrp_num, u8 link_num, bool *is_limit_crossed,
	time64_t *limit_crossed_time)
{
	sl_core_data_link_ucw_warn_limit_crossed_get(sl_core_link_get(ldev_num, lgrp_num, link_num),
		is_limit_crossed, limit_crossed_time);
}

void sl_core_link_ucw_warn_limit_crossed_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool is_limit_crossed)
{
	sl_core_data_link_ucw_warn_limit_crossed_set(sl_core_link_get(ldev_num, lgrp_num, link_num), is_limit_crossed);
}

void sl_core_link_ccw_warn_limit_crossed_get(u8 ldev_num, u8 lgrp_num, u8 link_num, bool *is_limit_crossed,
	time64_t *limit_crossed_time)
{
	sl_core_data_link_ccw_warn_limit_crossed_get(sl_core_link_get(ldev_num, lgrp_num, link_num),
		is_limit_crossed, limit_crossed_time);
}

void sl_core_link_ccw_warn_limit_crossed_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool is_limit_crossed)
{
	sl_core_data_link_ccw_warn_limit_crossed_set(sl_core_link_get(ldev_num, lgrp_num, link_num), is_limit_crossed);
}

bool sl_core_link_policy_is_keep_serdes_up_set(struct sl_core_link *core_link)
{
	bool is_policy_set;

	spin_lock(&core_link->serdes.data_lock);
	is_policy_set = (core_link->policy.options & SL_LINK_POLICY_OPT_KEEP_SERDES_UP) != 0;
	spin_unlock(&core_link->serdes.data_lock);

	return is_policy_set;
}

bool sl_core_link_policy_is_use_unsupported_cable_set(struct sl_core_link *core_link)
{
	bool is_policy_set;

	spin_lock(&core_link->serdes.data_lock);
	is_policy_set = (core_link->policy.options & SL_LINK_POLICY_OPT_USE_UNSUPPORTED_CABLE) != 0;
	spin_unlock(&core_link->serdes.data_lock);

	return is_policy_set;
}

struct sl_core_link_up_info *sl_core_link_up_info_get(struct sl_core_link *core_link,
	struct sl_core_link_up_info *link_up_info)
{
	spin_lock(&core_link->link.data_lock);
	link_up_info->state     = core_link->link.state;
	link_up_info->cause_map = core_link->link.last_up_fail_cause_map;
	link_up_info->info_map  = core_link->info_map;
	link_up_info->speed     = core_link->pcs.settings.speed;
	link_up_info->fec_mode  = core_link->fec.settings.mode;
	link_up_info->fec_type  = core_link->fec.settings.type;
	spin_unlock(&core_link->link.data_lock);

	return link_up_info;
}
