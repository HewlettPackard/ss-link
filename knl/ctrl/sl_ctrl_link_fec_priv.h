/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LINK_FEC_PRIV_H_
#define _SL_CTRL_LINK_FEC_PRIV_H_

#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "sl_core_link_fec.h"

struct sl_ctrl_link;

#define SL_CTRL_LINK_FEC_UCW_MANT   1
#define SL_CTRL_LINK_FEC_UCW_EXP  -10
#define SL_CTRL_LINK_FEC_CCW_MANT   2
#define SL_CTRL_LINK_FEC_CCW_EXP   -5

/* Normalize the rate, and check limit
 * l = limit
 * p = period_ms
 * c = [U/C]CW delta (count)
 * t = time = (p / 1000)
 *
 * l < (c / t); l * t < c; (l * p) < (c * 1000)
 */
#define SL_CTRL_LINK_FEC_LIMIT_CHECK(_limit, _count, _period) \
	((_limit && _period) ? (((u64)(_limit) * (_period)) <= ((_count) * 1000)) : 0)
#define SL_CTRL_LINK_FEC_UCW_LIMIT_CHECK(_limit, _info) \
	SL_CTRL_LINK_FEC_LIMIT_CHECK(_limit, (_info)->ucw, (_info)->period_ms)
#define SL_CTRL_LINK_FEC_CCW_LIMIT_CHECK(_limit, _info) \
	SL_CTRL_LINK_FEC_LIMIT_CHECK(_limit, (_info)->ccw, (_info)->period_ms)

struct sl_ctrl_link_fec_cntrs {
	struct sl_core_link_fec_cw_cntrs   cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;
	unsigned long                      timestamp;
};

struct sl_ctrl_link_fec_data {
	struct sl_ctrl_link_fec_cntrs  cntrs[2];
	struct sl_ctrl_link_fec_cntrs *curr_ptr;
	struct sl_ctrl_link_fec_cntrs *prev_ptr;
	struct sl_fec_info             info;
	struct sl_fec_tail             tail;
	spinlock_t                     lock;
};

struct sl_ctrl_link_fec_cache {
	struct sl_core_link_fec_cw_cntrs   cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;
	spinlock_t                         lock;
};

void                sl_ctrl_link_fec_data_store(struct sl_ctrl_link *ctrl_link,
						struct sl_core_link_fec_cw_cntrs *cw_cntrs,
						struct sl_core_link_fec_lane_cntrs *lane_cntrs,
						struct sl_core_link_fec_tail_cntrs *tail_cntrs);
void                sl_ctrl_link_fec_up_cache_clear(struct sl_ctrl_link *ctrl_link);
void                sl_ctrl_link_fec_up_cache_store(struct sl_ctrl_link *ctrl_link,
						    struct sl_core_link_fec_cw_cntrs *cw_cntrs,
						    struct sl_core_link_fec_lane_cntrs *lane_cntrs,
						    struct sl_core_link_fec_tail_cntrs *tail_cntrs);
void                sl_ctrl_link_fec_down_cache_clear(struct sl_ctrl_link *ctrl_link);
void                sl_ctrl_link_fec_down_cache_store(struct sl_ctrl_link *ctrl_link,
						      struct sl_core_link_fec_cw_cntrs *cw_cntrs,
						      struct sl_core_link_fec_lane_cntrs *lane_cntrs,
						      struct sl_core_link_fec_tail_cntrs *tail_cntrs);
void                sl_ctrl_link_fec_data_calc(struct sl_ctrl_link *ctrl_link);
int                 sl_ctrl_link_fec_data_check(struct sl_ctrl_link *ctrl_link);
struct sl_fec_info  sl_ctrl_link_fec_data_info_get(struct sl_ctrl_link *ctrl_link);
struct sl_fec_tail  sl_ctrl_link_fec_data_tail_get(struct sl_ctrl_link *ctrl_link);

int sl_ctrl_link_fec_up_cache_cw_cntrs_get(struct sl_ctrl_link *ctrl_link,
					   struct sl_core_link_fec_cw_cntrs *cw_cntrs);
int sl_ctrl_link_fec_up_cache_lane_cntrs_get(struct sl_ctrl_link *ctrl_link,
					     struct sl_core_link_fec_lane_cntrs *lane_cntrs);
int sl_ctrl_link_fec_up_cache_tail_cntrs_get(struct sl_ctrl_link *ctrl_link,
					     struct sl_core_link_fec_tail_cntrs *tail_cntrs);

int sl_ctrl_link_fec_down_cache_cw_cntrs_get(struct sl_ctrl_link *ctrl_link,
					     struct sl_core_link_fec_cw_cntrs *cw_cntrs);
int sl_ctrl_link_fec_down_cache_lane_cntrs_get(struct sl_ctrl_link *ctrl_link,
					       struct sl_core_link_fec_lane_cntrs *lane_cntrs);
int sl_ctrl_link_fec_down_cache_tail_cntrs_get(struct sl_ctrl_link *ctrl_link,
					       struct sl_core_link_fec_tail_cntrs *tail_cntrs);

void sl_ctrl_link_fec_mon_start(struct sl_ctrl_link *ctrl_link);
void sl_ctrl_link_fec_mon_timer_work(struct work_struct *work);
void sl_ctrl_link_fec_mon_timer(struct timer_list *timer);
void sl_ctrl_link_fec_mon_stop(struct sl_ctrl_link *ctrl_link);

u32 sl_ctrl_link_fec_limit_calc(struct sl_ctrl_link *ctrl_link, u32 mant, int exp);

#endif /* _SL_CTRL_LINK_FEC_PRIV_H_ */
