// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"
#include "data/sl_core_data_link.h"

#include "data/sl_ctrl_data_link.h"

#define LOG_NAME SL_CTRL_LINK_LOG_NAME

int sl_ctrl_data_link_state_get(struct sl_ctrl_link *ctrl_link, u32 *link_state)
{
	int rtn;
	u32 core_link_state;
	u32 ctrl_link_state;

	spin_lock(&ctrl_link->data_lock);
	rtn = sl_core_data_link_state_get(sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
							   ctrl_link->ctrl_lgrp->num,
							   ctrl_link->num), &core_link_state);
	ctrl_link_state = ctrl_link->state;
	spin_unlock(&ctrl_link->data_lock);

	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_link, LOG_NAME,
				      "sl_core_data_link_state_get failed (rtn = %d)", rtn);
		return rtn;
	}

	*link_state = (core_link_state == SL_CORE_LINK_STATE_AN) ? SL_LINK_STATE_AN : ctrl_link_state;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (link_state = %u %s)",
			*link_state,
			sl_link_state_str(*link_state));

	return 0;
}

int sl_ctrl_data_link_fec_up_settle_wait_ms_get(struct sl_ctrl_link *ctrl_link, u32 *fec_up_settle_wait_ms)
{
	spin_lock(&ctrl_link->data_lock);
	*fec_up_settle_wait_ms = ctrl_link->config.fec_up_settle_wait_ms;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (fec_up_settle_wait_ms = %d)", *fec_up_settle_wait_ms);

	return 0;
}

int sl_ctrl_data_link_fec_up_check_wait_ms_get(struct sl_ctrl_link *ctrl_link, s32 *fec_up_check_wait_ms)
{
	spin_lock(&ctrl_link->data_lock);
	*fec_up_check_wait_ms = ctrl_link->config.fec_up_check_wait_ms;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (fec_up_check_wait_ms = %d)", *fec_up_check_wait_ms);

	return 0;
}

int sl_ctrl_data_link_fec_up_ucw_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_up_ucw_limit)
{
	spin_lock(&ctrl_link->data_lock);
	*fec_up_ucw_limit = ctrl_link->config.fec_up_ucw_limit;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (fec_up_ucw_limit = %d)", *fec_up_ucw_limit);

	return 0;
}

int sl_ctrl_data_link_fec_up_ccw_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_up_ccw_limit)
{
	spin_lock(&ctrl_link->data_lock);
	*fec_up_ccw_limit = ctrl_link->config.fec_up_ccw_limit;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (fec_up_ccw_limit = %d)", *fec_up_ccw_limit);

	return 0;
}

int sl_ctrl_data_link_policy_fec_mon_period_ms_get(struct sl_ctrl_link *ctrl_link, s32 *mon_period_ms)
{
	spin_lock(&ctrl_link->data_lock);
	*mon_period_ms = ctrl_link->policy.fec_mon_period_ms;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (mon_period_ms = %d)", *mon_period_ms);

	return 0;
}

int sl_ctrl_data_link_policy_fec_mon_ucw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ucw_down_limit)
{
	spin_lock(&ctrl_link->data_lock);
	*mon_ucw_down_limit = ctrl_link->policy.fec_mon_ucw_down_limit;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (mon_ucw_down_limit = %d)", *mon_ucw_down_limit);

	return 0;
}

int sl_ctrl_data_link_policy_fec_mon_ucw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ucw_warn_limit)
{
	spin_lock(&ctrl_link->data_lock);
	*mon_ucw_warn_limit = ctrl_link->policy.fec_mon_ucw_warn_limit;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (mon_ucw_warn_limit = %d)", *mon_ucw_warn_limit);

	return 0;
}

int sl_ctrl_data_link_policy_fec_mon_ccw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ccw_down_limit)
{
	spin_lock(&ctrl_link->data_lock);
	*mon_ccw_down_limit = ctrl_link->policy.fec_mon_ccw_down_limit;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (mon_ccw_down_limit = %d)", *mon_ccw_down_limit);

	return 0;
}

int sl_ctrl_data_link_policy_fec_mon_ccw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ccw_warn_limit)
{
	spin_lock(&ctrl_link->data_lock);
	*mon_ccw_warn_limit = ctrl_link->policy.fec_mon_ccw_warn_limit;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (mon_ccw_warn_limit = %d)", *mon_ccw_warn_limit);

	return 0;
}

int sl_ctrl_data_link_fec_down_cache_ucw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ucw)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*ucw = ctrl_link->fec_down_cache.cw_cntrs.ucw;
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (ucw = %llu)", *ucw);

	return 0;
}

int sl_ctrl_data_link_fec_down_cache_ccw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ccw)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*ccw = ctrl_link->fec_down_cache.cw_cntrs.ccw;
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (ccw = %llu)", *ccw);

	return 0;
}

int sl_ctrl_data_link_fec_down_cache_gcw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *gcw)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*gcw = ctrl_link->fec_down_cache.cw_cntrs.gcw;
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (gcw = %llu)", *gcw);

	return 0;
}

int sl_ctrl_data_link_fec_down_cache_lane_cntr_get(struct sl_ctrl_link *ctrl_link, u8 lane_num, u64 *lane_cntr)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*lane_cntr = ctrl_link->fec_down_cache.lane_cntrs.lanes[lane_num];
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (lane_num = %d, lane_cntr = %llu)", lane_num, *lane_cntr);

	return 0;
}

int sl_ctrl_data_link_fec_down_cache_tail_cntr_get(struct sl_ctrl_link *ctrl_link, u8 bin_num, u64 *tail_cntr)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*tail_cntr = ctrl_link->fec_down_cache.tail_cntrs.ccw_bins[bin_num];
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (bin_num = %d, tail_cntr = %llu)", bin_num, *tail_cntr);

	return 0;
}

int sl_ctrl_data_link_fec_mon_period_ms_get(struct sl_ctrl_link *ctrl_link, u32 *mon_period_get)

{
	spin_lock(&ctrl_link->fec_data.lock);
	*mon_period_get = ctrl_link->fec_data.info.monitor.period_ms;
	spin_unlock(&ctrl_link->fec_data.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (mon_period_get = %u)", *mon_period_get);

	return 0;
}

int sl_ctrl_data_link_fec_mon_ucw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *ucw_down_limit)
{
	u32 period_ms;

	spin_lock(&ctrl_link->fec_data.lock);
	period_ms = ctrl_link->fec_data.info.monitor.period_ms;
	*ucw_down_limit = ctrl_link->fec_data.info.monitor.ucw_down_limit;
	spin_unlock(&ctrl_link->fec_data.lock);

	if (!period_ms) {
		sl_ctrl_log_warn(ctrl_link, LOG_NAME,
				 "fec_mon_ucw_down_limit_get failed (monitoring disabled)");
		return -EBADRQC;
	}

	return 0;
}

int sl_ctrl_data_link_fec_mon_ccw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *ccw_down_limit)
{
	u32 period_ms;

	spin_lock(&ctrl_link->fec_data.lock);
	period_ms = ctrl_link->fec_data.info.monitor.period_ms;
	*ccw_down_limit = ctrl_link->fec_data.info.monitor.ccw_down_limit;
	spin_unlock(&ctrl_link->fec_data.lock);

	if (!period_ms) {
		sl_ctrl_log_warn(ctrl_link, LOG_NAME,
				 "fec_mon_ccw_down_limit_get failed (monitoring disabled)");
		return -EBADRQC;
	}

	return 0;
}

int sl_ctrl_data_link_fec_mon_ccw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *ccw_warn_limit)
{
	u32 period_ms;

	spin_lock(&ctrl_link->fec_data.lock);
	period_ms = ctrl_link->fec_data.info.monitor.period_ms;
	*ccw_warn_limit = ctrl_link->fec_data.info.monitor.ccw_warn_limit;
	spin_unlock(&ctrl_link->fec_data.lock);

	if (!period_ms) {
		sl_ctrl_log_warn(ctrl_link, LOG_NAME,
				 "fec_mon_ccw_warn_limit_get failed (monitoring disabled)");
		return -EBADRQC;
	}

	return 0;
}

int sl_ctrl_data_link_fec_mon_ucw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *ucw_warn_limit)
{
	u32 period_ms;

	spin_lock(&ctrl_link->fec_data.lock);
	period_ms = ctrl_link->fec_data.info.monitor.period_ms;
	*ucw_warn_limit = ctrl_link->fec_data.info.monitor.ucw_warn_limit;
	spin_unlock(&ctrl_link->fec_data.lock);

	if (!period_ms) {
		sl_ctrl_log_warn(ctrl_link, LOG_NAME,
				 "fec_mon_ucw_warn_limit_get failed (monitoring disabled)");
		return -EBADRQC;
	}

	return 0;
}

int sl_ctrl_data_link_fec_up_cache_ucw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ucw)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*ucw = ctrl_link->fec_up_cache.cw_cntrs.ucw;
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (ucw = %llu)", *ucw);

	return 0;
}

int sl_ctrl_data_link_fec_up_cache_ccw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ccw)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*ccw = ctrl_link->fec_up_cache.cw_cntrs.ccw;
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (ccw = %llu)", *ccw);

	return 0;
}

int sl_ctrl_data_link_fec_up_cache_gcw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *gcw)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*gcw = ctrl_link->fec_up_cache.cw_cntrs.gcw;
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (gcw = %llu)", *gcw);

	return 0;
}

int sl_ctrl_data_link_fec_up_cache_lane_cntr_get(struct sl_ctrl_link *ctrl_link, u8 lane_num, u64 *lane_cntr)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*lane_cntr = ctrl_link->fec_up_cache.lane_cntrs.lanes[lane_num];
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (lane_num = %d, lane_cntr = %llu)", lane_num, *lane_cntr);

	return 0;
}

int sl_ctrl_data_link_fec_up_cache_tail_cntr_get(struct sl_ctrl_link *ctrl_link, u8 bin_num, u64 *tail_cntr)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*tail_cntr = ctrl_link->fec_up_cache.tail_cntrs.ccw_bins[bin_num];
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (bin_num = %d, tail_cntr = %llu)", bin_num, *tail_cntr);

	return 0;
}

int sl_ctrl_data_link_policy_options_get(struct sl_ctrl_link *ctrl_link, u32 *options)
{
	spin_lock(&ctrl_link->data_lock);
	*options = ctrl_link->policy.options;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (options = 0x%08x)", *options);

	return 0;
}

int sl_ctrl_data_link_up_timeout_ms_get(struct sl_ctrl_link *ctrl_link, u32 *link_up_timeout_ms)
{
	spin_lock(&ctrl_link->data_lock);
	*link_up_timeout_ms = ctrl_link->config.link_up_timeout_ms;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (link_up_timeout_ms = %d)", *link_up_timeout_ms);

	return 0;
}

int sl_ctrl_data_link_up_tries_max_get(struct sl_ctrl_link *ctrl_link, u32 *link_up_tries_max)
{
	spin_lock(&ctrl_link->data_lock);
	*link_up_tries_max = ctrl_link->config.link_up_tries_max;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (link_up_tries_max = %d)", *link_up_tries_max);

	return 0;
}

int sl_ctrl_data_link_config_options_get(struct sl_ctrl_link *ctrl_link, u32 *options)
{
	spin_lock(&ctrl_link->data_lock);
	*options = ctrl_link->config.options;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (options = 0x%08x)", *options);

	return 0;
}

int sl_ctrl_data_link_pause_map_get(struct sl_ctrl_link *ctrl_link, u32 *pause_map)
{
	spin_lock(&ctrl_link->data_lock);
	*pause_map = ctrl_link->config.pause_map;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (pause_map = 0x%x)", *pause_map);

	return 0;
}

int sl_ctrl_data_link_hpe_map_get(struct sl_ctrl_link *ctrl_link, u32 *hpe_map)
{
	spin_lock(&ctrl_link->data_lock);
	*hpe_map = ctrl_link->config.hpe_map;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (hpe_map = 0x%x)", *hpe_map);

	return 0;
}
