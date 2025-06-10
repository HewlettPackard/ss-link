// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024-2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_core_str.h"
#include "sl_core_llr.h"
#include "base/sl_core_timer_llr.h"
#include "base/sl_core_work_llr.h"
#include "base/sl_core_log.h"

#define LOG_NAME SL_CORE_TIMER_LLR_LOG_NAME

void sl_core_timer_llr_begin(struct sl_core_llr *core_llr, u32 timer_num)
{
	u32 llr_state;

	if (core_llr->timers[timer_num].data.timeout_ms == 0) {
		sl_core_log_warn_trace(core_llr, LOG_NAME,
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

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_SETTING_UP:
	case SL_CORE_LLR_STATE_STARTING:
		add_timer(&(core_llr->timers[timer_num].timer));
		spin_unlock(&core_llr->data_lock);
		return;
	default:
		sl_core_log_err(core_llr, LOG_NAME, "begin - invalid state (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return;
	}
}


/**
 * sl_core_timer_llr_timeout - Timer callback function executed in interrupt context.
 * @timer: Pointer to the timer_list structure associated with the timer.
 *
 * Context: Interrupt context.
 */
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

	queue_work(core_llr->core_lgrp->core_ldev->workqueue, &(core_llr->work[info->data.work_num]));
}

void sl_core_timer_llr_end(struct sl_core_llr *core_llr, u32 timer_num)
{
	sl_core_log_dbg(core_llr, LOG_NAME,
		"end - %s (llr = 0x%p, timer = %d)",
		core_llr->timers[timer_num].data.log, core_llr, timer_num);

	del_timer_sync(&(core_llr->timers[timer_num].timer));
}
