// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/workqueue.h>

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_lgrp_notif.h"
#include "sl_ctl_link.h"
#include "sl_ctl_link_counters.h"
#include "sl_ctl_link_fec_priv.h"

#define LOG_NAME SL_CTL_LINK_FEC_LOG_NAME

void sl_ctl_link_fec_data_store(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_cw_cntrs   *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	struct sl_ctl_link_fec_cntrs *tmp_cntrs_ptr;
	unsigned long                 irq_flags;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "data_store");

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);

	tmp_cntrs_ptr = ctl_link->fec_data.prev_ptr;
	ctl_link->fec_data.prev_ptr = ctl_link->fec_data.curr_ptr;
	ctl_link->fec_data.curr_ptr = tmp_cntrs_ptr;

	ctl_link->fec_data.curr_ptr->cw_cntrs   = *cw_cntrs;
	ctl_link->fec_data.curr_ptr->lane_cntrs = *lane_cntrs;
	ctl_link->fec_data.curr_ptr->tail_cntrs = *tail_cntrs;
	ctl_link->fec_data.curr_ptr->timestamp  = jiffies;

	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);
}

void sl_ctl_link_fec_up_cache_clear(struct sl_ctl_link *ctl_link)
{
	memset(&(ctl_link->fec_up_cache), 0, sizeof(struct sl_ctl_link_fec_cache));
}

void sl_ctl_link_fec_up_cache_store(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_cw_cntrs   *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	unsigned long irq_flags;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up_cache_store");

	spin_lock_irqsave(&ctl_link->fec_up_cache.lock, irq_flags);
	ctl_link->fec_up_cache.cw_cntrs   = *cw_cntrs;
	ctl_link->fec_up_cache.lane_cntrs = *lane_cntrs;
	ctl_link->fec_up_cache.tail_cntrs = *tail_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_up_cache.lock, irq_flags);
}

void sl_ctl_link_fec_down_cache_clear(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_down_cache.lock, irq_flags);
	memset(&(ctl_link->fec_down_cache), 0, sizeof(struct sl_ctl_link_fec_cache));
	spin_unlock_irqrestore(&ctl_link->fec_down_cache.lock, irq_flags);
}

void sl_ctl_link_fec_down_cache_store(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_cw_cntrs   *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	unsigned long irq_flags;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "down_cache_store");

	spin_lock_irqsave(&ctl_link->fec_down_cache.lock, irq_flags);
	ctl_link->fec_down_cache.cw_cntrs   = *cw_cntrs;
	ctl_link->fec_down_cache.lane_cntrs = *lane_cntrs;
	ctl_link->fec_down_cache.tail_cntrs = *tail_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_down_cache.lock, irq_flags);
}

void sl_ctl_link_fec_mon_start(struct sl_ctl_link *ctl_link)
{
	u32           period;
	unsigned long irq_flags;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "mon_start");

	spin_lock_irqsave(&ctl_link->config_lock, irq_flags);
	period = ctl_link->policy.fec_mon_period_ms;
	spin_unlock_irqrestore(&ctl_link->config_lock, irq_flags);
	if (!period) {
		sl_ctl_link_fec_mon_stop(ctl_link);
		return;
	}

	spin_lock_irqsave(&ctl_link->fec_mon_timer_lock, irq_flags);
	ctl_link->fec_mon_timer_stop = false;
	mod_timer(&ctl_link->fec_mon_timer, jiffies + msecs_to_jiffies(period));
	spin_unlock_irqrestore(&ctl_link->fec_mon_timer_lock, irq_flags);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor timer started (period = %ums)", period);
}

void sl_ctl_link_fec_data_calc(struct sl_ctl_link *ctl_link)
{
	int           x;
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);

	ctl_link->fec_data.info.ccw =
		ctl_link->fec_data.curr_ptr->cw_cntrs.ccw - ctl_link->fec_data.prev_ptr->cw_cntrs.ccw;
	ctl_link->fec_data.info.ucw =
		ctl_link->fec_data.curr_ptr->cw_cntrs.ucw - ctl_link->fec_data.prev_ptr->cw_cntrs.ucw;
	ctl_link->fec_data.info.gcw =
		ctl_link->fec_data.curr_ptr->cw_cntrs.gcw - ctl_link->fec_data.prev_ptr->cw_cntrs.gcw;
	for (x = 0; x < SL_CORE_LINK_FEC_NUM_LANES; ++x)
		ctl_link->fec_data.info.lanes[x] = ctl_link->fec_data.curr_ptr->lane_cntrs.lanes[x] -
			ctl_link->fec_data.prev_ptr->lane_cntrs.lanes[x];
	ctl_link->fec_data.info.period_ms =
		jiffies_to_msecs(ctl_link->fec_data.curr_ptr->timestamp - ctl_link->fec_data.prev_ptr->timestamp);

	for (x = 0; x < SL_CTL_NUM_CCW_BINS; ++x)
		ctl_link->fec_data.tail.ccw_bins[x] = ctl_link->fec_data.curr_ptr->tail_cntrs.ccw_bins[x] -
			ctl_link->fec_data.prev_ptr->tail_cntrs.ccw_bins[x];
	ctl_link->fec_data.tail.period_ms = ctl_link->fec_data.info.period_ms;

	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	if ((ctl_link->fec_data.info.ccw > 0) || (ctl_link->fec_data.info.ucw > 0)) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME,
			"data_calc (ccw = %llu, ucw = %llu, gcw = %llu, period = %ums)",
			ctl_link->fec_data.info.ccw, ctl_link->fec_data.info.ucw,
			ctl_link->fec_data.info.gcw, ctl_link->fec_data.info.period_ms);
		for (x = 0; x < SL_CTL_NUM_CCW_BINS; ++x)
			sl_ctl_log_dbg(ctl_link, LOG_NAME,
				"data_calc (bin %d = %llu)", x, ctl_link->fec_data.tail.ccw_bins[x]);
	}
}

struct sl_fec_info sl_ctl_link_fec_data_info_get(struct sl_ctl_link *ctl_link)
{
	struct sl_fec_info fec_info;
	unsigned long      irq_flags;

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	fec_info = ctl_link->fec_data.info;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	return fec_info;
}

struct sl_fec_tails sl_ctl_link_fec_data_tail_get(struct sl_ctl_link *ctl_link)
{
	struct sl_fec_tails fec_tail;
	unsigned long       irq_flags;

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	fec_tail = ctl_link->fec_data.tail;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	return fec_tail;
}

int sl_ctl_link_fec_up_cache_cw_cntrs_get(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_up_cache.lock, irq_flags);
	*cw_cntrs = ctl_link->fec_up_cache.cw_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_up_cache.lock, irq_flags);

	return 0;
}

int sl_ctl_link_fec_up_cache_lane_cntrs_get(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_up_cache.lock, irq_flags);
	*lane_cntrs = ctl_link->fec_up_cache.lane_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_up_cache.lock, irq_flags);

	return 0;
}

int sl_ctl_link_fec_up_cache_tail_cntrs_get(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_up_cache.lock, irq_flags);
	*tail_cntrs = ctl_link->fec_up_cache.tail_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_up_cache.lock, irq_flags);

	return 0;
}

int sl_ctl_link_fec_down_cache_cw_cntrs_get(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_down_cache.lock, irq_flags);
	*cw_cntrs = ctl_link->fec_down_cache.cw_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_down_cache.lock, irq_flags);

	return 0;
}

int sl_ctl_link_fec_down_cache_lane_cntrs_get(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_down_cache.lock, irq_flags);
	*lane_cntrs = ctl_link->fec_down_cache.lane_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_down_cache.lock, irq_flags);

	return 0;
}

int sl_ctl_link_fec_down_cache_tail_cntrs_get(struct sl_ctl_link *ctl_link,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->fec_down_cache.lock, irq_flags);
	*tail_cntrs = ctl_link->fec_down_cache.tail_cntrs;
	spin_unlock_irqrestore(&ctl_link->fec_down_cache.lock, irq_flags);

	return 0;
}

int sl_ctl_link_fec_data_check(struct sl_ctl_link *ctl_link)
{
	int                rtn;
	s32                ccw_down_limit;
	s32                ccw_warn_limit;
	s32                ucw_down_limit;
	s32                ucw_warn_limit;
	struct sl_fec_info fec_info;
	unsigned long      irq_flags;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "data check");

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ccw_down_limit = ctl_link->fec_data.info.monitor.ccw_down_limit;
	ccw_warn_limit = ctl_link->fec_data.info.monitor.ccw_warn_limit;
	ucw_down_limit = ctl_link->fec_data.info.monitor.ucw_down_limit;
	ucw_warn_limit = ctl_link->fec_data.info.monitor.ucw_warn_limit;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	fec_info = ctl_link->fec_data.info;

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"data check config (ccw_down = %d, ucw_down = %d, ccw_warn = %d, ucw_warn = %d)",
		ccw_down_limit, ucw_down_limit, ccw_warn_limit, ucw_warn_limit);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"data check info (UCW = %llu, CCW = %llu, GCW = %llu, period = %ums)",
		fec_info.ucw, fec_info.ccw, fec_info.gcw, fec_info.period_ms);

	if (SL_CTL_LINK_FEC_UCW_LIMIT_CHECK(ucw_down_limit, &fec_info)) {
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"UCW exceeded down limit (UCW = %llu, CCW = %llu)",
			fec_info.ucw, fec_info.ccw);

		ctl_link->down_cause = SL_LINK_DOWN_CAUSE_UCW;
		if (!queue_work(ctl_link->ctl_lgrp->ctl_ldev->workq, &ctl_link->down_work))
			sl_ctl_log_warn(ctl_link, LOG_NAME, "UCW down_work already scheduled");

		return -EIO;
	}

	if (SL_CTL_LINK_FEC_CCW_LIMIT_CHECK(ccw_down_limit, &fec_info)) {
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"CCW exceeded down limit (UCW = %llu, CCW = %llu)",
			fec_info.ucw, fec_info.ccw);

		ctl_link->down_cause = SL_LINK_DOWN_CAUSE_CCW;
		if (!queue_work(ctl_link->ctl_lgrp->ctl_ldev->workq, &ctl_link->down_work))
			sl_ctl_log_warn(ctl_link, LOG_NAME, "CCW down_work already scheduled");

		return -EIO;
	}

	if (SL_CTL_LINK_FEC_UCW_LIMIT_CHECK(ucw_warn_limit, &fec_info)) {
		sl_ctl_log_warn(ctl_link, LOG_NAME,
			"UCW exceeded warning limit (UCW = %llu, CCW = %llu)",
			fec_info.ucw, fec_info.ccw);
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
				  SL_LGRP_NOTIF_LINK_UCW_WARN, NULL, 0, 0);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"ctl_lgrp_notif_enqueue failed [%d]", rtn);
	}

	if (SL_CTL_LINK_FEC_CCW_LIMIT_CHECK(ccw_warn_limit, &fec_info)) {
		sl_ctl_log_warn(ctl_link, LOG_NAME,
			"CCW exceeded warning limit (UCW = %llu, CCW = %llu)",
			fec_info.ucw, fec_info.ccw);

		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
				  SL_LGRP_NOTIF_LINK_CCW_WARN, NULL, 0, 0);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"ctl_lgrp_notif_enqueue failed [%d]", rtn);
	}

	return 0;
}

void sl_ctl_link_fec_mon_timer_work(struct work_struct *work)
{
	int                                  rtn;
	struct sl_ctl_link                  *ctl_link;
	unsigned long                        irq_flags;
	u32                                  period;
	bool                                 stop;
	struct sl_core_link_fec_cw_cntrs     cw_cntrs;
	struct sl_core_link_fec_lane_cntrs   lane_cntrs;
	struct sl_core_link_fec_tail_cntrs   tail_cntrs;

	ctl_link = container_of(work, struct sl_ctl_link, fec_mon_timer_work);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor timer work");

	spin_lock_irqsave(&ctl_link->config_lock, irq_flags);
	period = ctl_link->policy.fec_mon_period_ms;
	spin_unlock_irqrestore(&ctl_link->config_lock, irq_flags);
	if (!period) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor period zero");
		return;
	}

	spin_lock_irqsave(&ctl_link->fec_mon_timer_lock, irq_flags);
	stop = ctl_link->fec_mon_timer_stop;
	spin_unlock_irqrestore(&ctl_link->fec_mon_timer_lock, irq_flags);
	if (stop) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor stopped");
		return;
	}

	rtn = sl_core_link_fec_data_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn) {
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"core_link_fec_tail_cntrs_get failed [%d]", rtn);
		goto start_mon;
	}

	sl_ctl_link_fec_data_store(ctl_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_ctl_link_fec_data_calc(ctl_link);

	rtn = sl_ctl_link_fec_data_check(ctl_link);
	if (rtn) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "check failed [%d]", rtn);
		return;
	}

	spin_lock_irqsave(&ctl_link->config_lock, irq_flags);
	period = ctl_link->policy.fec_mon_period_ms;
	spin_unlock_irqrestore(&ctl_link->config_lock, irq_flags);
	if (!period) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor period zero");
		return;
	}

	spin_lock_irqsave(&ctl_link->fec_mon_timer_lock, irq_flags);
	stop = ctl_link->fec_mon_timer_stop;
	spin_unlock_irqrestore(&ctl_link->fec_mon_timer_lock, irq_flags);
	if (stop) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor stopped");
		return;
	}

start_mon:
	SL_CTL_LINK_COUNTER_INC(ctl_link, FEC_MON_START);
	mod_timer(&ctl_link->fec_mon_timer, jiffies + msecs_to_jiffies(period));
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor started");
}

void sl_ctl_link_fec_mon_timer(struct timer_list *timer)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = from_timer(ctl_link, timer, fec_mon_timer);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor timer");

	if (!queue_work(ctl_link->ctl_lgrp->ctl_ldev->workq, &(ctl_link->fec_mon_timer_work)))
		sl_ctl_log_warn(ctl_link, LOG_NAME, "fec mon timer work already queued");
}

void sl_ctl_link_fec_mon_stop(struct sl_ctl_link *ctl_link)
{
	bool          stop;
	int           rtn;
	unsigned long irq_flags;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor stop");

	spin_lock_irqsave(&ctl_link->fec_mon_timer_lock, irq_flags);
	stop = ctl_link->fec_mon_timer_stop;
	ctl_link->fec_mon_timer_stop = true;
	spin_unlock_irqrestore(&ctl_link->fec_mon_timer_lock, irq_flags);

	if (stop) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "monitor already stopped");
		return;
	}

	rtn = del_timer_sync(&ctl_link->fec_mon_timer);
	if (rtn < 0)
		sl_ctl_log_warn(ctl_link, LOG_NAME, "del_timer_sync failed [%d]", rtn);
}