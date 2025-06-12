// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>

#include "sl_ctl_link.h"
#include "sl_ctl_link_counters.h"

#define SL_CTL_LINK_COUNTER_INIT(_link, _counter)          \
	do {                                                   \
		atomic_set(&(_link)->counters[_counter].count, 0); \
		(_link)->counters[_counter].name = #_counter;      \
	} while (0)

int sl_ctl_link_counters_init(struct sl_ctl_link *ctl_link)
{
	if (ctl_link->counters)
		return -EALREADY;

	ctl_link->counters = kzalloc(sizeof(struct sl_ctl_link_counter) * SL_CTL_LINK_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctl_link->counters)
		return -ENOMEM;

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_CMD);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_RETRY);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_FAIL);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_DOWN_CMD);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_DOWN);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_CANCEL_CMD);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_CANCELED);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_RESET_CMD);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_FAULT);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_RECOVERING);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_CCW_WARN_CROSSED);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UCW_WARN_CROSSED);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_HW_AN_ATTEMPT);

	return 0;
}

int sl_ctl_link_counters_get(struct sl_ctl_link *ctl_link, u32 counter)
{
	return atomic_read(&ctl_link->counters[counter].count);
}

void sl_ctl_link_counters_del(struct sl_ctl_link *ctl_link)
{
	if (ctl_link)
		kfree(ctl_link->counters);
}
