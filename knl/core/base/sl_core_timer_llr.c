// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_core_llr.h"
#include "base/sl_core_timer_llr.h"
#include "base/sl_core_work_llr.h"
#include "base/sl_core_log.h"

#define LOG_NAME SL_CORE_TIMER_LLR_LOG_NAME

void sl_core_timer_llr_begin(struct sl_core_llr *core_llr, u32 timer_num)
{
	if (core_llr->timers[timer_num].data.timeout_ms == 0) {
		sl_core_log_warn(core_llr, LOG_NAME,
			"begin - %s timeout is 0", core_llr->timers[timer_num].data.log);
		return;
	}

	core_llr->timers[timer_num].timer.expires =
		jiffies + msecs_to_jiffies(core_llr->timers[timer_num].data.timeout_ms);

	sl_core_log_dbg(core_llr, LOG_NAME,
		"begin - %s (llr = 0x%p, timer = %d, timeout = %dms, jiffies = %ld)",
		core_llr->timers[timer_num].data.log, core_llr, timer_num,
		core_llr->timers[timer_num].data.timeout_ms,
		core_llr->timers[timer_num].timer.expires);

	if (sl_core_llr_is_canceled_or_timed_out(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME,
			"begin - canceled (timer = %u)", timer_num);
		return;
	}

	add_timer(&(core_llr->timers[timer_num].timer));
}

void sl_core_timer_llr_timeout(struct timer_list *timer)
{
	struct sl_core_timer_llr_info *info;
	struct sl_core_llr            *core_llr;

	info = from_timer(info, timer, timer);
	core_llr = info->data.core_llr;

	sl_core_log_dbg(core_llr, LOG_NAME,
		"timeout %s (llr = 0x%p, timer = %u, work = %u)",
		info->data.log, core_llr,
		info->data.timer_num, info->data.work_num);

	sl_core_work_llr_queue(core_llr, info->data.work_num);
}

int sl_core_timer_llr_end(struct sl_core_llr *core_llr, u32 timer_num)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME,
		"end - %s (llr = 0x%p, timer = %d)",
		core_llr->timers[timer_num].data.log, core_llr, timer_num);

	rtn = del_timer_sync(&(core_llr->timers[timer_num].timer));
	if (rtn < 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"end - %s - del_timer_sync failed [%d]",
			core_llr->timers[timer_num].data.log, rtn);
	return rtn;
}
