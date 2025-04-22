/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_TIMER_LLR_H_
#define _SL_CORE_TIMER_LLR_H_

#include <linux/timer.h>

struct sl_core_llr;

enum {
	SL_CORE_TIMER_LLR_SETUP = 0,
	SL_CORE_TIMER_LLR_START,

	SL_CORE_TIMER_LLR_COUNT           /* must be last */
};

#define SL_CORE_TIMER_LLR_LOG_SIZE 30

struct sl_core_timer_llr_data {
	struct sl_core_llr *core_llr;
	u32                 timer_num;
	u32                 work_num;
	u32                 timeout_ms;
	char                log[SL_CORE_TIMER_LLR_LOG_SIZE + 1];
};

struct sl_core_timer_llr_info {
	struct timer_list             timer;
	struct sl_core_timer_llr_data data;
};

void sl_core_timer_llr_begin(struct sl_core_llr *core_llr, u32 timer_num);
void sl_core_timer_llr_timeout(struct timer_list *timer);
void sl_core_timer_llr_end(struct sl_core_llr *core_llr, u32 timer_num);

#endif /* _SL_CORE_TIMER_LLR_H_ */
