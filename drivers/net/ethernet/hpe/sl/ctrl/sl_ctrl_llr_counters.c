// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "sl_ctrl_llr.h"
#include "sl_ctrl_llr_counters.h"

#define SL_CTRL_LLR_COUNTER_INIT(_llr, _counter) \
	(_llr)->counters[_counter].name = #_counter

int sl_ctrl_llr_counters_init(struct sl_ctrl_llr *ctrl_llr)
{
	if (ctrl_llr->counters)
		return -EALREADY;

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

int sl_ctrl_llr_counter_get(struct sl_ctrl_llr *ctrl_llr, u32 counter)
{
	return atomic_read(&ctrl_llr->counters[counter].count);
}

void sl_ctrl_llr_counters_del(struct sl_ctrl_llr *ctrl_llr)
{
	kfree(ctrl_llr->counters);
}
