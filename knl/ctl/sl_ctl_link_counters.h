/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LINK_COUNTERS_H_
#define _SL_CTL_LINK_COUNTERS_H_

struct sl_ctl_link;

enum sl_ctl_link_counters {
	LINK_UP_CMD,              /* command to bring the link up   */
	LINK_UP_RETRY,            /* link up retry                  */
	LINK_UP,                  /* link is up                     */
	LINK_UP_FAIL,             /* link up failed                 */
	LINK_DOWN_CMD,            /* command to bring the link down */
	LINK_DOWN,                /* link is down                   */
	LINK_UP_CANCEL_CMD,       /* link up cancel commanded       */
	LINK_UP_CANCELED,         /* link up cancel completed       */
	LINK_RESET_CMD,           /* command to reset the link      */
	LINK_FAULT,               /* link fault                     */
	LINK_RECOVERING,          /* FIXME: need to implement this  */
	LINK_CCW_WARN_CROSSED,    /* ccw warn limit crossed         */
	LINK_UCW_WARN_CROSSED,    /* ucw warn limit crossed         */
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
