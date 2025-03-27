// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024, 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_llr.h"
#include "base/sl_core_work_llr.h"

#define LOG_NAME SL_CORE_WORK_LLR_LOG_NAME

void sl_core_work_llr_queue(struct sl_core_llr *core_llr, u32 work_num)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "queue - (core_link = 0x%p, work_num = %d)", core_llr, work_num);

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "canceled (core_llr = 0x%p)", core_llr);
		return;
	}

	if (!queue_work(core_llr->core_lgrp->core_ldev->workqueue,
		&(core_llr->work[work_num])))
		sl_core_log_warn(core_llr, LOG_NAME, "already queued (work_num = %u)", work_num);
}
