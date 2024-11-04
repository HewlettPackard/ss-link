// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_work_link.h"

#define LOG_NAME SL_CORE_WORK_LINK_LOG_NAME

void sl_core_work_link_queue(struct sl_core_link *core_link, u32 work_num)
{
	sl_core_log_dbg(core_link, LOG_NAME, "queue - (core_link = 0x%p, work_num = %d)", core_link, work_num);

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "canceled (core_link = 0x%p)", core_link);
		return;
	}

	if (!queue_work(core_link->core_lgrp->core_ldev->workqueue,
		&(core_link->work[work_num])))
		sl_core_log_warn(core_link, LOG_NAME, "already queued (work_num = %u)", work_num);
}

void sl_core_work_link_flush(struct sl_core_link *core_link, u32 work_num)
{
	bool waited;

	sl_core_log_dbg(core_link, LOG_NAME, "flush - (core_link = 0x%p, work_num = %d)", core_link, work_num);

	waited = flush_work(&(core_link->work[work_num]));

	sl_core_log_dbg(core_link, LOG_NAME, "flush - (waited = %s)", waited ? "true" : "false");
}
