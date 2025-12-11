/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LLR_COUNTERS_H_
#define _SL_CTRL_LLR_COUNTERS_H_

#include <linux/types.h>
#include <linux/atomic.h>

struct sl_ctrl_llr;

enum sl_ctrl_llr_counters {
	LLR_SETUP_CMD,            /* command to setup llr   */
	LLR_SETUP,                /* llr setup is succeeded */
	LLR_SETUP_TIMEOUT,        /* llr setup is timed out */
	LLR_SETUP_FAIL,           /* llr setup is failed    */
	LLR_CONFIGURED,           /* llr is configured      */
	LLR_START_CMD,            /* command to start llr   */
	LLR_RUNNING,              /* llr is running         */
	LLR_START_TIMEOUT,        /* llr start is timed out */
	LLR_START_FAIL,           /* llr start is failed    */
	LLR_STOP_CMD,             /* command to stop llr    */
	LLR_STOP_FAIL,            /* llr stop is failed     */
	SL_CTRL_LLR_COUNTERS_COUNT
};

enum sl_ctrl_llr_cause_counters {
	LLR_CAUSE_SETUP_CONFIG,         /* llr setup config invalid             */
	LLR_CAUSE_SETUP_INTR_ENABLE,    /* llr setup interrupt enable failed    */
	LLR_CAUSE_SETUP_TIMEOUT,        /* llr setup timeout                    */
	LLR_CAUSE_START_INTR_ENABLE,    /* llr start interrupt enable failed    */
	LLR_CAUSE_START_TIMEOUT,        /* llr start timeout                    */
	LLR_CAUSE_COMMAND,              /* running llr was commanded to stop    */
	LLR_CAUSE_CANCELED,             /* llr operation was canceled by client */
	SL_CTRL_LLR_CAUSE_COUNTERS_COUNT
};

struct sl_ctrl_llr_counter {
	atomic_t  count;
	char     *name;
};

#define SL_CTRL_LLR_COUNTER_INC(_llr, _counter) \
	atomic_inc(&(_llr)->counters[_counter].count)

int  sl_ctrl_llr_counters_init(struct sl_ctrl_llr *ctrl_llr);
void sl_ctrl_llr_counters_del(struct sl_ctrl_llr *ctrl_llr);
int  sl_ctrl_llr_counter_get(struct sl_ctrl_llr *ctrl_llr, u32 counter, int *count);

int  sl_ctrl_llr_cause_counters_init(struct sl_ctrl_llr *ctrl_llr);
void sl_ctrl_llr_cause_counters_del(struct sl_ctrl_llr *ctrl_llr);
int  sl_ctrl_llr_cause_counter_get(struct sl_ctrl_llr *ctrl_llr, u32 counter, int *count);

void sl_ctrl_llr_cause_counter_inc(struct sl_core_llr *core_llr, u32 cause_map);

#endif /* _SL_CTRL_LLR_COUNTERS_H_ */
