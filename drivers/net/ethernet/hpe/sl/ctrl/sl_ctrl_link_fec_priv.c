// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/workqueue.h>

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_lgrp_notif.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_link_priv.h"
#include "sl_ctrl_link_counters.h"
#include "sl_ctrl_link_fec_priv.h"
#include "sl_core_link.h"

#define SL_CTRL_LINK_UCW_LIMIT_MAX_CHANCES 2
#define SL_CTRL_LINK_CCW_LIMIT_MAX_CHANCES 2
#define SL_CTRL_LINK_FEC_MON_PERIOD_MS     500
#define LOG_NAME SL_CTRL_LINK_FEC_LOG_NAME

void sl_ctrl_link_fec_data_store(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_cw_cntrs   *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	struct sl_ctrl_link_fec_cntrs *tmp_cntrs_ptr;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "data_store");

	spin_lock(&ctrl_link->fec_data.lock);

	tmp_cntrs_ptr = ctrl_link->fec_data.prev_ptr;
	ctrl_link->fec_data.prev_ptr = ctrl_link->fec_data.curr_ptr;
	ctrl_link->fec_data.curr_ptr = tmp_cntrs_ptr;

	ctrl_link->fec_data.curr_ptr->cw_cntrs   = *cw_cntrs;
	ctrl_link->fec_data.curr_ptr->lane_cntrs = *lane_cntrs;
	ctrl_link->fec_data.curr_ptr->tail_cntrs = *tail_cntrs;
	ctrl_link->fec_data.curr_ptr->timestamp  = jiffies;

	spin_unlock(&ctrl_link->fec_data.lock);
}

void sl_ctrl_link_fec_up_cache_clear(struct sl_ctrl_link *ctrl_link)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	memset(&(ctrl_link->fec_up_cache.cw_cntrs),   0, sizeof(struct sl_core_link_fec_cw_cntrs));
	memset(&(ctrl_link->fec_up_cache.lane_cntrs), 0, sizeof(struct sl_core_link_fec_lane_cntrs));
	memset(&(ctrl_link->fec_up_cache.tail_cntrs), 0, sizeof(struct sl_core_link_fec_tail_cntrs));
	spin_unlock(&ctrl_link->fec_up_cache.lock);
}

void sl_ctrl_link_fec_up_cache_store(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_cw_cntrs   *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "up_cache_store");

	spin_lock(&ctrl_link->fec_up_cache.lock);
	ctrl_link->fec_up_cache.cw_cntrs   = *cw_cntrs;
	ctrl_link->fec_up_cache.lane_cntrs = *lane_cntrs;
	ctrl_link->fec_up_cache.tail_cntrs = *tail_cntrs;
	spin_unlock(&ctrl_link->fec_up_cache.lock);
}

void sl_ctrl_link_fec_down_cache_clear(struct sl_ctrl_link *ctrl_link)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	memset(&(ctrl_link->fec_down_cache.cw_cntrs),   0, sizeof(struct sl_core_link_fec_cw_cntrs));
	memset(&(ctrl_link->fec_down_cache.lane_cntrs), 0, sizeof(struct sl_core_link_fec_lane_cntrs));
	memset(&(ctrl_link->fec_down_cache.tail_cntrs), 0, sizeof(struct sl_core_link_fec_tail_cntrs));
	spin_unlock(&ctrl_link->fec_down_cache.lock);
}

void sl_ctrl_link_fec_down_cache_store(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_cw_cntrs   *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "down_cache_store");

	spin_lock(&ctrl_link->fec_down_cache.lock);
	ctrl_link->fec_down_cache.cw_cntrs   = *cw_cntrs;
	ctrl_link->fec_down_cache.lane_cntrs = *lane_cntrs;
	ctrl_link->fec_down_cache.tail_cntrs = *tail_cntrs;
	spin_unlock(&ctrl_link->fec_down_cache.lock);
}

static void sl_ctrl_link_fec_mon_limits_calc(struct sl_ctrl_link *ctrl_link)
{
	struct sl_link_policy link_policy;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "mon limits calc");

	spin_lock(&ctrl_link->fec_data.lock);

	link_policy = ctrl_link->policy;

	ctrl_link->fec_data.info.monitor.period_ms = (link_policy.fec_mon_period_ms < 0) ?
		SL_CTRL_LINK_FEC_MON_PERIOD_MS : link_policy.fec_mon_period_ms;

	if (link_policy.fec_mon_ccw_down_limit < 0)
		ctrl_link->fec_data.info.monitor.ccw_down_limit = 0;
	else
		ctrl_link->fec_data.info.monitor.ccw_down_limit = link_policy.fec_mon_ccw_down_limit;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"mon limits calc (ccw_down_limit = %d)",
		ctrl_link->fec_data.info.monitor.ccw_down_limit);

	if (link_policy.fec_mon_ccw_warn_limit < 0)
		ctrl_link->fec_data.info.monitor.ccw_warn_limit =
			sl_ctrl_link_fec_limit_calc(ctrl_link,
						    SL_CTRL_LINK_FEC_CCW_MANT, SL_CTRL_LINK_FEC_CCW_EXP) >> 1;
	else
		ctrl_link->fec_data.info.monitor.ccw_warn_limit = link_policy.fec_mon_ccw_warn_limit;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"mon limits calc (ccw_warn_limit = %d)",
		ctrl_link->fec_data.info.monitor.ccw_warn_limit);

	if (ctrl_link->fec_data.info.monitor.ccw_down_limit &&
		(ctrl_link->fec_data.info.monitor.ccw_warn_limit > ctrl_link->fec_data.info.monitor.ccw_down_limit))
		sl_ctrl_log_warn(ctrl_link, LOG_NAME,
			"CCW warning limit set greater than down limit (%d > %d)",
			ctrl_link->fec_data.info.monitor.ccw_warn_limit,
			ctrl_link->fec_data.info.monitor.ccw_down_limit);

	if (link_policy.fec_mon_ucw_down_limit < 0) {
		ctrl_link->fec_data.info.monitor.ucw_down_limit =
			sl_ctrl_link_fec_limit_calc(ctrl_link, SL_CTRL_LINK_FEC_UCW_MANT, SL_CTRL_LINK_FEC_UCW_EXP);
	} else {
		ctrl_link->fec_data.info.monitor.ucw_down_limit = link_policy.fec_mon_ucw_down_limit;
	}

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"mon limits calc (ucw_down_limit = %d)",
		ctrl_link->fec_data.info.monitor.ucw_down_limit);

	if (link_policy.fec_mon_ucw_warn_limit < 0)
		ctrl_link->fec_data.info.monitor.ucw_warn_limit =
			sl_ctrl_link_fec_limit_calc(ctrl_link, SL_CTRL_LINK_FEC_UCW_MANT,
						    SL_CTRL_LINK_FEC_UCW_EXP) >> 1;
	else
		ctrl_link->fec_data.info.monitor.ucw_warn_limit = link_policy.fec_mon_ucw_warn_limit;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"mon limits calc (ucw_warn_limit = %d)",
		ctrl_link->fec_data.info.monitor.ucw_warn_limit);

	if (ctrl_link->fec_data.info.monitor.ucw_down_limit &&
		(ctrl_link->fec_data.info.monitor.ucw_warn_limit > ctrl_link->fec_data.info.monitor.ucw_down_limit))
		sl_ctrl_log_warn(ctrl_link, LOG_NAME,
			"CCW warning limit set greater than down limit (%d > %d)",
			ctrl_link->fec_data.info.monitor.ucw_warn_limit,
			ctrl_link->fec_data.info.monitor.ucw_down_limit);

	spin_unlock(&ctrl_link->fec_data.lock);
}

void sl_ctrl_link_fec_mon_start(struct sl_ctrl_link *ctrl_link)
{
	u32 period;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "mon_start");

	sl_ctrl_link_fec_mon_limits_calc(ctrl_link);

	spin_lock(&ctrl_link->fec_data.lock);
	period = ctrl_link->fec_data.info.monitor.period_ms;
	spin_unlock(&ctrl_link->fec_data.lock);
	if (!period) {
		sl_ctrl_link_fec_mon_stop(ctrl_link);
		cancel_work_sync(&ctrl_link->fec_mon_timer_work);
		return;
	}

	spin_lock(&ctrl_link->fec_mon_timer_lock);
	ctrl_link->fec_mon_timer_stop = false;
	mod_timer(&ctrl_link->fec_mon_timer, jiffies + msecs_to_jiffies(period));
	spin_unlock(&ctrl_link->fec_mon_timer_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor timer started (period = %ums)", period);
}

void sl_ctrl_link_fec_data_calc(struct sl_ctrl_link *ctrl_link)
{
	int x;

	spin_lock(&ctrl_link->fec_data.lock);

	ctrl_link->fec_data.info.ccw =
		ctrl_link->fec_data.curr_ptr->cw_cntrs.ccw - ctrl_link->fec_data.prev_ptr->cw_cntrs.ccw;
	ctrl_link->fec_data.info.ucw =
		ctrl_link->fec_data.curr_ptr->cw_cntrs.ucw - ctrl_link->fec_data.prev_ptr->cw_cntrs.ucw;
	ctrl_link->fec_data.info.gcw =
		ctrl_link->fec_data.curr_ptr->cw_cntrs.gcw - ctrl_link->fec_data.prev_ptr->cw_cntrs.gcw;
	for (x = 0; x < SL_CORE_LINK_FEC_NUM_LANES; ++x)
		ctrl_link->fec_data.info.lanes[x] = ctrl_link->fec_data.curr_ptr->lane_cntrs.lanes[x] -
			ctrl_link->fec_data.prev_ptr->lane_cntrs.lanes[x];
	ctrl_link->fec_data.info.period_ms =
		jiffies_to_msecs(ctrl_link->fec_data.curr_ptr->timestamp - ctrl_link->fec_data.prev_ptr->timestamp);

	for (x = 0; x < SL_CTRL_NUM_CCW_BINS; ++x)
		ctrl_link->fec_data.tail.ccw_bins[x] = ctrl_link->fec_data.curr_ptr->tail_cntrs.ccw_bins[x] -
			ctrl_link->fec_data.prev_ptr->tail_cntrs.ccw_bins[x];
	ctrl_link->fec_data.tail.period_ms = ctrl_link->fec_data.info.period_ms;

	spin_unlock(&ctrl_link->fec_data.lock);

	if ((ctrl_link->fec_data.info.ccw > 0) || (ctrl_link->fec_data.info.ucw > 0)) {
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"data_calc (ccw = %llu, ucw = %llu, gcw = %llu, period = %ums)",
			ctrl_link->fec_data.info.ccw, ctrl_link->fec_data.info.ucw,
			ctrl_link->fec_data.info.gcw, ctrl_link->fec_data.info.period_ms);
		for (x = 0; x < SL_CTRL_NUM_CCW_BINS; ++x)
			sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
				"data_calc (bin %d = %llu)", x, ctrl_link->fec_data.tail.ccw_bins[x]);
	}
}

struct sl_fec_info sl_ctrl_link_fec_data_info_get(struct sl_ctrl_link *ctrl_link)
{
	struct sl_fec_info fec_info;

	spin_lock(&ctrl_link->fec_data.lock);
	fec_info = ctrl_link->fec_data.info;
	spin_unlock(&ctrl_link->fec_data.lock);

	return fec_info;
}

struct sl_fec_tail sl_ctrl_link_fec_data_tail_get(struct sl_ctrl_link *ctrl_link)
{
	struct sl_fec_tail fec_tail;

	spin_lock(&ctrl_link->fec_data.lock);
	fec_tail = ctrl_link->fec_data.tail;
	spin_unlock(&ctrl_link->fec_data.lock);

	return fec_tail;
}

int sl_ctrl_link_fec_up_cache_cw_cntrs_get(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*cw_cntrs = ctrl_link->fec_up_cache.cw_cntrs;
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	return 0;
}

int sl_ctrl_link_fec_up_cache_lane_cntrs_get(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*lane_cntrs = ctrl_link->fec_up_cache.lane_cntrs;
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	return 0;
}

int sl_ctrl_link_fec_up_cache_tail_cntrs_get(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	spin_lock(&ctrl_link->fec_up_cache.lock);
	*tail_cntrs = ctrl_link->fec_up_cache.tail_cntrs;
	spin_unlock(&ctrl_link->fec_up_cache.lock);

	return 0;
}

int sl_ctrl_link_fec_down_cache_cw_cntrs_get(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*cw_cntrs = ctrl_link->fec_down_cache.cw_cntrs;
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	return 0;
}

int sl_ctrl_link_fec_down_cache_lane_cntrs_get(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*lane_cntrs = ctrl_link->fec_down_cache.lane_cntrs;
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	return 0;
}

int sl_ctrl_link_fec_down_cache_tail_cntrs_get(struct sl_ctrl_link *ctrl_link,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	spin_lock(&ctrl_link->fec_down_cache.lock);
	*tail_cntrs = ctrl_link->fec_down_cache.tail_cntrs;
	spin_unlock(&ctrl_link->fec_down_cache.lock);

	return 0;
}

static bool sl_ctrl_link_fec_ucw_chance_limit_check(struct sl_ctrl_link *ctrl_link, s32 limit, struct sl_fec_info *info)
{
	ctrl_link->fec_ucw_chance = SL_CTRL_LINK_FEC_UCW_LIMIT_CHECK(limit, info) ? ctrl_link->fec_ucw_chance + 1 : 0;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "fec_ucw_chance_limit_check (limit = %d, UCW = %llu, ucw_chance = %u)",
		limit, info->ucw, ctrl_link->fec_ucw_chance);

	return ctrl_link->fec_ucw_chance > SL_CTRL_LINK_UCW_LIMIT_MAX_CHANCES;
}

static bool sl_ctrl_link_fec_ccw_chance_limit_check(struct sl_ctrl_link *ctrl_link, s32 limit, struct sl_fec_info *info)
{
	ctrl_link->fec_ccw_chance = SL_CTRL_LINK_FEC_CCW_LIMIT_CHECK(limit, info) ? ctrl_link->fec_ccw_chance + 1 : 0;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "fec_ccw_chance_limit_check (limit = %d, CCW = %llu, ccw_chance = %u)",
		limit, info->ccw, ctrl_link->fec_ccw_chance);

	return ctrl_link->fec_ccw_chance > SL_CTRL_LINK_CCW_LIMIT_MAX_CHANCES;
}

int sl_ctrl_link_fec_data_check(struct sl_ctrl_link *ctrl_link)
{
	int                rtn;
	s32                ccw_down_limit;
	s32                ccw_warn_limit;
	s32                ucw_down_limit;
	s32                ucw_warn_limit;
	struct sl_fec_info fec_info;
	bool               is_limit_crossed;
	time64_t           limit_crossed_time;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "data check");

	spin_lock(&ctrl_link->fec_data.lock);
	ccw_down_limit = ctrl_link->fec_data.info.monitor.ccw_down_limit;
	ccw_warn_limit = ctrl_link->fec_data.info.monitor.ccw_warn_limit;
	ucw_down_limit = ctrl_link->fec_data.info.monitor.ucw_down_limit;
	ucw_warn_limit = ctrl_link->fec_data.info.monitor.ucw_warn_limit;
	spin_unlock(&ctrl_link->fec_data.lock);

	fec_info = ctrl_link->fec_data.info;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"data check config (ccw_down = %d, ucw_down = %d, ccw_warn = %d, ucw_warn = %d)",
		ccw_down_limit, ucw_down_limit, ccw_warn_limit, ucw_warn_limit);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"data check info (UCW = %llu, CCW = %llu, GCW = %llu, period = %ums)",
		fec_info.ucw, fec_info.ccw, fec_info.gcw, fec_info.period_ms);

	if (sl_ctrl_link_fec_ucw_chance_limit_check(ctrl_link, ucw_down_limit, &fec_info)) {
		sl_ctrl_log_err(ctrl_link, LOG_NAME,
			"UCW exceeded down limit (UCW = %llu, CCW = %llu, ucw_chance = %u)",
			fec_info.ucw, fec_info.ccw, ctrl_link->fec_ucw_chance);
		ctrl_link->fec_ucw_chance = 0;
		rtn = sl_ctrl_link_async_down(ctrl_link, SL_LINK_DOWN_CAUSE_UCW_MAP);
		if (rtn) {
			sl_ctrl_log_err_trace(ctrl_link, LOG_NAME,
				"data check ctrl_link_async_down failed [%d]", rtn);
			return rtn;
		}
		return -EIO;
	}

	if (sl_ctrl_link_fec_ccw_chance_limit_check(ctrl_link, ccw_down_limit, &fec_info)) {
		sl_ctrl_log_err(ctrl_link, LOG_NAME,
			"CCW exceeded down limit (UCW = %llu, CCW = %llu, ccw_chance = %u)",
			fec_info.ucw, fec_info.ccw, ctrl_link->fec_ccw_chance);
		ctrl_link->fec_ccw_chance = 0;
		rtn = sl_ctrl_link_async_down(ctrl_link, SL_LINK_DOWN_CAUSE_CCW_MAP);
		if (rtn) {
			sl_ctrl_log_err_trace(ctrl_link, LOG_NAME, "link_down_and_notify failed [%d]", rtn);
			return rtn;
		}
		return -EIO;
	}

	if (SL_CTRL_LINK_FEC_UCW_LIMIT_CHECK(ucw_warn_limit, &fec_info)) {
		sl_core_link_ucw_warn_limit_crossed_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
			ctrl_link->ctrl_lgrp->num, ctrl_link->num, &is_limit_crossed, &limit_crossed_time);
		if (!is_limit_crossed) {
			sl_ctrl_log_warn(ctrl_link, LOG_NAME,
				"UCW exceeded warn limit (UCW = %llu, CCW = %llu)",
				fec_info.ucw, fec_info.ccw);
			rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
				SL_LGRP_NOTIF_LINK_UCW_WARN, NULL, 0);
			if (rtn)
				sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					"data check ctrl_lgrp_notif_enqueue failed [%d]", rtn);
		}
		sl_core_link_ucw_warn_limit_crossed_set(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
			ctrl_link->ctrl_lgrp->num, ctrl_link->num, true);
		SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_UCW_WARN_CROSSED);
	}

	if (SL_CTRL_LINK_FEC_CCW_LIMIT_CHECK(ccw_warn_limit, &fec_info)) {
		sl_core_link_ccw_warn_limit_crossed_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
			ctrl_link->ctrl_lgrp->num, ctrl_link->num, &is_limit_crossed, &limit_crossed_time);
		if (!is_limit_crossed) {
			sl_ctrl_log_warn(ctrl_link, LOG_NAME,
				"CCW exceeded warn limit (UCW = %llu, CCW = %llu)",
				fec_info.ucw, fec_info.ccw);
			rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
				  SL_LGRP_NOTIF_LINK_CCW_WARN, NULL, 0);
			if (rtn)
				sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					"data check ctrl_lgrp_notif_enqueue failed [%d]", rtn);
		}
		sl_core_link_ccw_warn_limit_crossed_set(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
			ctrl_link->ctrl_lgrp->num, ctrl_link->num, true);
		SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_CCW_WARN_CROSSED);
	}

	return 0;
}

void sl_ctrl_link_fec_mon_timer_work(struct work_struct *work)
{
	int                                  rtn;
	struct sl_ctrl_link                 *ctrl_link;
	s32                                  period;
	bool                                 stop;
	struct sl_core_link_fec_cw_cntrs     cw_cntrs;
	struct sl_core_link_fec_lane_cntrs   lane_cntrs;
	struct sl_core_link_fec_tail_cntrs   tail_cntrs;

	ctrl_link = container_of(work, struct sl_ctrl_link, fec_mon_timer_work);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor timer work");

	spin_lock(&ctrl_link->fec_data.lock);
	period = ctrl_link->fec_data.info.monitor.period_ms;
	spin_unlock(&ctrl_link->fec_data.lock);
	if (!period) {
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor period zero");
		return;
	}

	spin_lock(&ctrl_link->fec_mon_timer_lock);
	stop = ctrl_link->fec_mon_timer_stop;
	spin_unlock(&ctrl_link->fec_mon_timer_lock);
	if (stop) {
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor stopped");
		return;
	}

	rtn = sl_core_link_fec_data_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
		ctrl_link->ctrl_lgrp->num, ctrl_link->num, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_link, LOG_NAME,
			"core_link_fec_tail_cntrs_get failed [%d]", rtn);
		goto start_mon;
	}

	sl_ctrl_link_fec_data_store(ctrl_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_ctrl_link_fec_data_calc(ctrl_link);

	rtn = sl_ctrl_link_fec_data_check(ctrl_link);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_link, LOG_NAME, "check failed [%d]", rtn);
		return;
	}

	spin_lock(&ctrl_link->fec_data.lock);
	period = ctrl_link->fec_data.info.monitor.period_ms;
	spin_unlock(&ctrl_link->fec_data.lock);
	if (!period) {
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor period zero");
		return;
	}

	spin_lock(&ctrl_link->fec_mon_timer_lock);
	stop = ctrl_link->fec_mon_timer_stop;
	spin_unlock(&ctrl_link->fec_mon_timer_lock);
	if (stop) {
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor stopped");
		return;
	}

start_mon:
	mod_timer(&ctrl_link->fec_mon_timer, jiffies + msecs_to_jiffies(period));
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor started");
}

void sl_ctrl_link_fec_mon_timer(struct timer_list *timer)
{
	struct sl_ctrl_link *ctrl_link;

	ctrl_link = from_timer(ctrl_link, timer, fec_mon_timer);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor timer");

	if (!queue_work(ctrl_link->ctrl_lgrp->ctrl_ldev->workq, &(ctrl_link->fec_mon_timer_work)))
		sl_ctrl_log_warn(ctrl_link, LOG_NAME, "fec mon timer work already queued");
}

void sl_ctrl_link_fec_mon_stop(struct sl_ctrl_link *ctrl_link)
{
	bool stop;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor stop");

	spin_lock(&ctrl_link->fec_mon_timer_lock);
	stop = ctrl_link->fec_mon_timer_stop;
	ctrl_link->fec_mon_timer_stop = true;
	spin_unlock(&ctrl_link->fec_mon_timer_lock);

	if (stop) {
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "monitor already stopped");
		return;
	}

	del_timer_sync(&ctrl_link->fec_mon_timer);
}

#define SL_CTRL_LINK_FEC_LIMIT_25   25781250000ULL
#define SL_CTRL_LINK_FEC_LIMIT_50   53125000000ULL
#define SL_CTRL_LINK_FEC_LIMIT_100 106250000000ULL
u32 sl_ctrl_link_fec_limit_calc(struct sl_ctrl_link *ctrl_link, u32 mant, int exp)
{
	u64                 limit;
	int                 x;
	struct sl_link_caps link_caps;

	sl_core_link_caps_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
		ctrl_link->ctrl_lgrp->num, ctrl_link->num, &link_caps);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"fec limit calc (tech_map = 0x%X, mant = %u, exp = %d)",
		link_caps.tech_map, mant, exp);

	if (link_caps.tech_map & SL_LGRP_CONFIG_TECH_CK_400G)
		limit = SL_CTRL_LINK_FEC_LIMIT_100 << 2;
	else if (link_caps.tech_map & SL_LGRP_CONFIG_TECH_CK_200G)
		limit = SL_CTRL_LINK_FEC_LIMIT_100 << 1;
	else if (link_caps.tech_map & SL_LGRP_CONFIG_TECH_BS_200G)
		limit = SL_CTRL_LINK_FEC_LIMIT_50 << 2;
	else if (link_caps.tech_map & SL_LGRP_CONFIG_TECH_CK_100G)
		limit = SL_CTRL_LINK_FEC_LIMIT_100;
	else if (link_caps.tech_map & SL_LGRP_CONFIG_TECH_BJ_100G)
		limit = SL_CTRL_LINK_FEC_LIMIT_25 << 2;
	else if (link_caps.tech_map & SL_LGRP_CONFIG_TECH_CD_100G)
		limit = SL_CTRL_LINK_FEC_LIMIT_50 << 1;
	else if (link_caps.tech_map & SL_LGRP_CONFIG_TECH_CD_50G)
		limit = SL_CTRL_LINK_FEC_LIMIT_50;
	else
		limit = SL_CTRL_LINK_FEC_LIMIT_100;

	limit *= mant;
	for (x = exp; x < 0; ++x)
		limit /= 10;

	if (limit > INT_MAX)
		return INT_MAX;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"fec limit calc (limit = %llu)", limit);

	return limit;
}
