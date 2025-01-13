/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LINK_COUNTERS_H_
#define _SL_CTL_LINK_COUNTERS_H_

struct sl_ctl_link;

enum sl_ctl_link_counters {
	LINK_UP_START,
	LINK_UP_RETRY,
	LINK_UP,
	LINK_UP_FAIL,
	LINK_UP_NOTIFIER,

	LINK_DOWN_CLIENT,
	LINK_DOWN,
	LINK_DOWN_FAIL,

	LINK_RESET_START,
	LINK_RESET,
	LINK_RESET_FAIL,

	LINK_ASYNC_FAULT_NOTIFIER,
	LINK_ASYNC_DOWN,
	LINK_RECOVERING,
	LINK_AUTO_RESTART_REQ,
	LINK_AUTO_RESTART,

	FEC_MON_START,

	SL_CTL_LINK_COUNTERS_COUNT
};

struct sl_ctl_link_counter {
	atomic_t  count;
	char     *name;
};

#define SL_CTL_LINK_COUNTER_INC(_link, _counter) \
	atomic_inc(&(_link)->counters[_counter].count)

int  sl_ctl_link_counters_init(struct sl_ctl_link *ctl_link);
void sl_ctl_link_counters_del(struct sl_ctl_link *ctl_link);
int  sl_ctl_link_counters_get(struct sl_ctl_link *ctl_link, u32 counter);

#endif /* _SL_CTL_LINK_COUNTERS_H_ */
