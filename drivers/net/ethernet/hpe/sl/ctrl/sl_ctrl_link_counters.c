// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>

#include "sl_ctrl_link.h"
#include "sl_ctrl_link_counters.h"

#define SL_CTRL_LINK_COUNTER_INIT(_link, _counter)          \
	do {                                                   \
		atomic_set(&(_link)->counters[_counter].count, 0); \
		(_link)->counters[_counter].name = #_counter;      \
	} while (0)

int sl_ctrl_link_counters_init(struct sl_ctrl_link *ctrl_link)
{
	if (ctrl_link->counters)
		return -EALREADY;

	ctrl_link->counters = kzalloc(sizeof(struct sl_ctrl_link_counter) * SL_CTRL_LINK_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctrl_link->counters)
		return -ENOMEM;

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_CMD);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_RETRY);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_FAIL);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_CMD);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_CANCEL_CMD);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_CANCELED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_RESET_CMD);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_FAULT);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_RECOVERING);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_CCW_WARN_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UCW_WARN_CROSSED);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_CCW_LIMIT_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_UCW_LIMIT_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_CCW_CAUSE);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_UCW_CAUSE);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_FAIL_CCW_LIMIT_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_FAIL_UCW_LIMIT_CROSSED);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_HW_AN_ATTEMPT);

	return 0;
}

int sl_ctrl_link_counters_get(struct sl_ctrl_link *ctrl_link, u32 counter)
{
	return atomic_read(&ctrl_link->counters[counter].count);
}

void sl_ctrl_link_counters_del(struct sl_ctrl_link *ctrl_link)
{
	if (ctrl_link)
		kfree(ctrl_link->counters);
}
