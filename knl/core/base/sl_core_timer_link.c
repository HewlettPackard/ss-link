// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_timer_link.h"
#include "base/sl_core_work_link.h"
#include "base/sl_core_log.h"
#include "data/sl_core_data_link.h"

#include "hw/sl_core_hw_intr.h"

#define LOG_NAME SL_CORE_TIMER_LINK_LOG_NAME

void sl_core_timer_link_begin(struct sl_core_link *core_link, u32 timer_num)
{
	if (core_link->timers[timer_num].data.timeout_ms == 0) {
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"begin - %s timeout is 0", core_link->timers[timer_num].data.log);
		return;
	}

	core_link->timers[timer_num].timer.expires =
		jiffies + msecs_to_jiffies(core_link->timers[timer_num].data.timeout_ms);

	sl_core_log_dbg(core_link, LOG_NAME,
		"begin - %s (link = 0x%p, timer = %d, timeout = %dms, jiffies = %ld)",
		core_link->timers[timer_num].data.log, core_link, timer_num,
		core_link->timers[timer_num].data.timeout_ms,
		core_link->timers[timer_num].timer.expires);

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME,
			"begin - canceled (timer = %u)", timer_num);
		return;
	}
	spin_lock(&core_link->link.data_lock);
	switch (core_link->link.state) {
	case SL_CORE_LINK_STATE_CANCELING:
		sl_core_log_dbg(core_link, LOG_NAME, "begin - canceled (timer = %u)", timer_num);
		spin_unlock(&core_link->link.data_lock);
		return;
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "begin - timeout (timer = %u)", timer_num);
		spin_unlock(&core_link->link.data_lock);
		return;
	default:
		add_timer(&(core_link->timers[timer_num].timer));
		spin_unlock(&core_link->link.data_lock);
		return;
	}
}

void sl_core_timer_link_up_timeout(struct timer_list *timer)
{
	struct sl_core_timer_link_info *info;
	struct sl_core_link            *core_link;
	u32                             link_state;

	info = from_timer(info, timer, timer);
	core_link = info->data.link;

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		core_link->link.state = SL_CORE_LINK_STATE_TIMEOUT;
		core_link->link.last_up_fail_cause_map |= SL_LINK_DOWN_CAUSE_TIMEOUT_MAP;
		core_link->link.last_up_fail_time       = ktime_get_real_seconds();
		sl_core_log_dbg(core_link, LOG_NAME, "link up timeout - timeout");
		spin_unlock(&core_link->link.data_lock);
		break;
	default:
		sl_core_log_err(core_link, LOG_NAME, "link up timeout - invalid state (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return;
	}

	if (!queue_work(core_link->core_lgrp->core_ldev->workqueue,
		&(core_link->work[info->data.work_num])))
		sl_core_log_warn(core_link, LOG_NAME, "already queued (work_num = %u)", info->data.work_num);
}

void sl_core_timer_link_timeout(struct timer_list *timer)
{
	struct sl_core_timer_link_info *info;
	struct sl_core_link            *core_link;

	info = from_timer(info, timer, timer);
	core_link = info->data.link;

	if ((info->data.timer_num != SL_CORE_TIMER_LINK_UP_CHECK) &&
		(info->data.timer_num != SL_CORE_TIMER_LINK_UP_FEC_SETTLE) &&
		(info->data.timer_num != SL_CORE_TIMER_LINK_UP_FEC_CHECK))
		sl_core_log_err_trace(core_link, LOG_NAME,
			"timeout %s (link = 0x%p, timer = %u, work = %u)",
			info->data.log, core_link,
			info->data.timer_num, info->data.work_num);

	sl_core_work_link_queue(core_link, info->data.work_num);
}

void sl_core_timer_link_end(struct sl_core_link *core_link, u32 timer_num)
{
	sl_core_log_dbg(core_link, LOG_NAME,
		"end - %s (link = 0x%p, timer = %d)",
		core_link->timers[timer_num].data.log, core_link, timer_num);

	del_timer_sync(&(core_link->timers[timer_num].timer));
}
