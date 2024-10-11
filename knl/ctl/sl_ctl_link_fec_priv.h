/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LINK_FEC_PRIV_H_
#define _SL_CTL_LINK_FEC_PRIV_H_

#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "sl_core_link_fec.h"

struct sl_ctl_link;

/* Normalize the rate, and check limit
 * p = limit
 * c = [U/C]CW delta (count)
 * t = time = (jiffies / HZ)
 *
 * p < (c / t); p < (c / (jiffies / HZ)); (p * jiffies) < (c * HZ)
 */
#define SL_CTL_LINK_FEC_LIMIT_CHECK(_limit, _count, _period) \
	((_limit && _period) ? (((u64)(_limit) * (_period)) <= ((_count) * HZ)) : 0)
#define SL_CTL_LINK_FEC_UCW_LIMIT_CHECK(_limit, _info) \
	SL_CTL_LINK_FEC_LIMIT_CHECK(_limit, (_info)->ucw, (_info)->period_ms)
#define SL_CTL_LINK_FEC_CCW_LIMIT_CHECK(_limit, _info) \
	SL_CTL_LINK_FEC_LIMIT_CHECK(_limit, (_info)->ccw, (_info)->period_ms)

struct sl_ctl_link_fec_cntrs {
	struct sl_core_link_fec_cw_cntrs   cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;
	unsigned long                      timestamp;
};

struct sl_ctl_link_fec_data {
	struct sl_ctl_link_fec_cntrs  cntrs[2];
	struct sl_ctl_link_fec_cntrs *curr_ptr;
	struct sl_ctl_link_fec_cntrs *prev_ptr;
	struct sl_fec_info            info;
	struct sl_fec_tails           tail;
	spinlock_t                    lock;
};

struct sl_ctl_link_fec_cache {
	struct sl_core_link_fec_cw_cntrs   cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;
	spinlock_t                         lock;
};

void                sl_ctl_link_fec_data_store(struct sl_ctl_link *ctl_link,
					       struct sl_core_link_fec_cw_cntrs *cw_cntrs,
					       struct sl_core_link_fec_lane_cntrs *lane_cntrs,
					       struct sl_core_link_fec_tail_cntrs *tail_cntrs);
void                sl_ctl_link_fec_up_cache_clear(struct sl_ctl_link *ctl_link);
void                sl_ctl_link_fec_up_cache_store(struct sl_ctl_link *ctl_link,
						   struct sl_core_link_fec_cw_cntrs *cw_cntrs,
						   struct sl_core_link_fec_lane_cntrs *lane_cntrs,
						   struct sl_core_link_fec_tail_cntrs *tail_cntrs);
void                sl_ctl_link_fec_down_cache_clear(struct sl_ctl_link *ctl_link);
void                sl_ctl_link_fec_down_cache_store(struct sl_ctl_link *ctl_link,
						     struct sl_core_link_fec_cw_cntrs *cw_cntrs,
						     struct sl_core_link_fec_lane_cntrs *lane_cntrs,
						     struct sl_core_link_fec_tail_cntrs *tail_cntrs);
void                sl_ctl_link_fec_data_calc(struct sl_ctl_link *ctl_link);
int                 sl_ctl_link_fec_data_check(struct sl_ctl_link *ctl_link);
struct sl_fec_info  sl_ctl_link_fec_data_info_get(struct sl_ctl_link *ctl_link);
struct sl_fec_tails sl_ctl_link_fec_data_tail_get(struct sl_ctl_link *ctl_link);

int sl_ctl_link_fec_up_cache_cw_cntrs_get(struct sl_ctl_link *ctl_link,
					  struct sl_core_link_fec_cw_cntrs *cw_cntrs);
int sl_ctl_link_fec_up_cache_lane_cntrs_get(struct sl_ctl_link *ctl_link,
					    struct sl_core_link_fec_lane_cntrs *lane_cntrs);
int sl_ctl_link_fec_up_cache_tail_cntrs_get(struct sl_ctl_link *ctl_link,
					    struct sl_core_link_fec_tail_cntrs *tail_cntrs);

int sl_ctl_link_fec_down_cache_cw_cntrs_get(struct sl_ctl_link *ctl_link,
					    struct sl_core_link_fec_cw_cntrs *cw_cntrs);
int sl_ctl_link_fec_down_cache_lane_cntrs_get(struct sl_ctl_link *ctl_link,
					      struct sl_core_link_fec_lane_cntrs *lane_cntrs);
int sl_ctl_link_fec_down_cache_tail_cntrs_get(struct sl_ctl_link *ctl_link,
					      struct sl_core_link_fec_tail_cntrs *tail_cntrs);

void sl_ctl_link_fec_mon_start(struct sl_ctl_link *ctl_link);
void sl_ctl_link_fec_mon_timer_work(struct work_struct *work);
void sl_ctl_link_fec_mon_timer(struct timer_list *timer);
void sl_ctl_link_fec_mon_stop(struct sl_ctl_link *ctl_link);

#endif /* _SL_CTL_LINK_FEC_PRIV_H_ */
