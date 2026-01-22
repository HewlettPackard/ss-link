// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "sl_ctrl_llr.h"
#include "sl_ctrl_llr_counters.h"
#include "base/sl_ctrl_log.h"

#define LOG_NAME SL_CTRL_LLR_LOG_NAME

#define SL_CTRL_LLR_COUNTER_INIT(_llr, _counter) \
	(_llr)->counters[_counter].name = #_counter

#define SL_CTRL_LLR_CAUSE_COUNTER_INIT(_llr, _counter) \
	(_llr)->cause_counters[_counter].name = #_counter

#define SL_CTRL_LLR_CAUSE_COUNTER_INC(_llr, _counter) \
	atomic_inc(&(_llr)->cause_counters[_counter].count)

int sl_ctrl_llr_counters_init(struct sl_ctrl_llr *ctrl_llr)
{
	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "llr counters init");

	ctrl_llr->counters = kzalloc(sizeof(*ctrl_llr->counters) * SL_CTRL_LLR_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctrl_llr->counters)
		return -ENOMEM;

	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_SETUP_CMD);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_SETUP);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_SETUP_TIMEOUT);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_SETUP_FAIL);

	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CONFIGURED);

	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_START_CMD);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_RUNNING);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_START_TIMEOUT);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_START_FAIL);

	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_STOP_CMD);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_STOP_FAIL);

	return 0;
}

int sl_ctrl_llr_cause_counters_init(struct sl_ctrl_llr *ctrl_llr)
{
	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "llr cause counters init");

	ctrl_llr->cause_counters = kzalloc(sizeof(*ctrl_llr->cause_counters) *
					   SL_CTRL_LLR_CAUSE_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctrl_llr->cause_counters)
		return -ENOMEM;

	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CAUSE_SETUP_CONFIG);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CAUSE_SETUP_INTR_ENABLE);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CAUSE_SETUP_TIMEOUT);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CAUSE_START_INTR_ENABLE);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CAUSE_START_TIMEOUT);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CAUSE_COMMAND);
	SL_CTRL_LLR_COUNTER_INIT(ctrl_llr, LLR_CAUSE_CANCELED);

	return 0;
}

int sl_ctrl_llr_counter_get(struct sl_ctrl_llr *ctrl_llr, u32 counter, int *count)
{
	*count = atomic_read(&ctrl_llr->counters[counter].count);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (counter = %u %s, count = %d)", counter,
			ctrl_llr->counters[counter].name, *count);

	return 0;
}

int sl_ctrl_llr_cause_counter_get(struct sl_ctrl_llr *ctrl_llr, u32 counter, int *count)
{
	*count = atomic_read(&ctrl_llr->cause_counters[counter].count);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (cause counter = %u %s, count = %d)", counter,
			ctrl_llr->cause_counters[counter].name, *count);

	return 0;
}

void sl_ctrl_llr_counters_del(struct sl_ctrl_llr *ctrl_llr)
{
	kfree(ctrl_llr->counters);
}

void sl_ctrl_llr_cause_counters_del(struct sl_ctrl_llr *ctrl_llr)
{
	kfree(ctrl_llr->cause_counters);
}

void sl_ctrl_llr_cause_counter_inc(struct sl_core_llr *core_llr, unsigned long cause_map)
{
	unsigned long       which;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(core_llr->core_lgrp->core_ldev->num, core_llr->core_lgrp->num, core_llr->num);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "llr cause counter inc");

	BUILD_BUG_ON(SL_CTRL_LLR_CAUSE_COUNTERS_COUNT > 32);

	for_each_set_bit(which, &cause_map, sizeof(cause_map) * BITS_PER_BYTE) {
		switch (BIT(which)) {
		case SL_LLR_FAIL_CAUSE_SETUP_CONFIG:
			SL_CTRL_LLR_CAUSE_COUNTER_INC(ctrl_llr, LLR_CAUSE_SETUP_CONFIG);
			break;
		case SL_LLR_FAIL_CAUSE_SETUP_INTR_ENABLE:
			SL_CTRL_LLR_CAUSE_COUNTER_INC(ctrl_llr, LLR_CAUSE_SETUP_INTR_ENABLE);
			break;
		case SL_LLR_FAIL_CAUSE_SETUP_TIMEOUT:
			SL_CTRL_LLR_CAUSE_COUNTER_INC(ctrl_llr, LLR_CAUSE_SETUP_TIMEOUT);
			break;
		case SL_LLR_FAIL_CAUSE_START_INTR_ENABLE:
			SL_CTRL_LLR_CAUSE_COUNTER_INC(ctrl_llr, LLR_CAUSE_START_INTR_ENABLE);
			break;
		case SL_LLR_FAIL_CAUSE_START_TIMEOUT:
			SL_CTRL_LLR_CAUSE_COUNTER_INC(ctrl_llr, LLR_CAUSE_START_TIMEOUT);
			break;
		default:
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "cause_counter_inc unknown cause (bit = %lu)", which);
			break;
		}
	}
}
