/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_TIMER_LINK_H_
#define _SL_CORE_TIMER_LINK_H_

#include <linux/timer.h>

struct sl_core_link;

enum {
	SL_CORE_TIMER_LINK_UP         = 0,
	SL_CORE_TIMER_LINK_UP_CHECK,
	SL_CORE_TIMER_LINK_UP_FEC_SETTLE,
	SL_CORE_TIMER_LINK_UP_FEC_CHECK,
	SL_CORE_TIMER_LINK_UP_HIGH_POWER,
	SL_CORE_TIMER_LINK_AN_LP_CAPS_GET,

	SL_CORE_TIMER_LINK_COUNT           /* must be last */
};

#define SL_CORE_TIMER_LINK_LOG_SIZE 30

struct sl_core_timer_link_data {
	struct sl_core_link *link;
	u32                  timer_num;
	u32                  work_num;
	u32                  timeout_ms;
	char                 log[SL_CORE_TIMER_LINK_LOG_SIZE + 1];
};

struct sl_core_timer_link_info {
	struct timer_list              timer;
	struct sl_core_timer_link_data data;
};

void sl_core_timer_link_begin(struct sl_core_link *link, u32 timer_num);
void sl_core_timer_link_timeout(struct timer_list *timer);
void sl_core_timer_link_end(struct sl_core_link *core_link, u32 timer_num);

#endif /* _SL_CORE_TIMER_LINK_H_ */
