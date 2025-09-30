/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LLR_COUNTERS_H_
#define _SL_CTRL_LLR_COUNTERS_H_

#include <linux/types.h>
#include <linux/atomic.h>

struct sl_ctrl_llr;

enum sl_ctrl_llr_counters {
	LLR_SETUP_CMD,            /* command to setup llr    */
	LLR_SETUP,                /* llr setup is succeeded  */
	LLR_SETUP_TIMEOUT,        /* llr setup is timed out  */
	LLR_SETUP_FAIL,           /* llr setup is failed     */
	LLR_CONFIGURED,           /* llr is configured       */
	LLR_START_CMD,            /* command to start llr    */
	LLR_RUNNING,              /* llr is running          */
	LLR_START_TIMEOUT,        /* llr start is timed out  */
	LLR_START_FAIL,           /* llr start is failed     */
	LLR_STOP_CMD,             /* command to stop llr     */
	LLR_STOP_FAIL,            /* llr stop is failed      */
	SL_CTRL_LLR_COUNTERS_COUNT
};

struct sl_ctrl_llr_counter {
	atomic_t  count;
	char     *name;
};

#define SL_CTRL_LLR_COUNTER_INC(_llr, _counter) \
	atomic_inc(&(_llr)->counters[_counter].count)

int  sl_ctrl_llr_counters_init(struct sl_ctrl_llr *ctrl_llr);
void sl_ctrl_llr_counters_del(struct sl_ctrl_llr *ctrl_llr);
int  sl_ctrl_llr_counter_get(struct sl_ctrl_llr *ctrl_llr, u32 counter);

#endif /* _SL_CTRL_LLR_COUNTERS_H_ */
