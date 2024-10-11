// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>

#include "sl_ctl_link.h"
#include "sl_ctl_link_counters.h"

#define SL_CTL_LINK_COUNTER_INIT(_link, _counter)                  \
	do {                                                       \
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

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_START);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_RETRY);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_FAIL);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_NOTIFIER);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_UP_WORK);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_DOWN_CLIENT);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_DOWN);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_DOWN_FAIL);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_RESET_START);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_RESET);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_RESET_FAIL);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_ASYNC_UP_NOTIFIER);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_ASYNC_FAULT_NOTIFIER);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_ASYNC_DOWN);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_RECOVERING);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_AUTO_RESTART_REQ);
	SL_CTL_LINK_COUNTER_INIT(ctl_link, LINK_AUTO_RESTART);

	SL_CTL_LINK_COUNTER_INIT(ctl_link, FEC_MON_START);

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
