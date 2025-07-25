/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LINK_H_
#define _SL_CTL_LINK_H_

#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>

#include <linux/sl_link.h>
#include <linux/sl_lgrp.h>

#include "sl_ctl_link_fec_priv.h"

#define SL_LINK_DOWN_CAUSE_COMMAND_MAP (      \
		SL_LINK_DOWN_ORIGIN_ASYNC   | \
		SL_LINK_DOWN_CAUSE_COMMAND  | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_UP_TRIES_MAP (     \
		SL_LINK_DOWN_CAUSE_UP_TRIES | \
		SL_LINK_DOWN_ORIGIN_LINK_UP | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_UCW_MAP (          \
		SL_LINK_DOWN_CAUSE_UCW      | \
		SL_LINK_DOWN_RETRYABLE      | \
		SL_LINK_DOWN_ORIGIN_ASYNC)
#define SL_LINK_DOWN_CAUSE_CCW_MAP (          \
		SL_LINK_DOWN_CAUSE_CCW      | \
		SL_LINK_DOWN_RETRYABLE      | \
		SL_LINK_DOWN_ORIGIN_ASYNC)

struct sl_ctl_link_fecl_kobj {
	struct sl_ctl_link *ctl_link;
	u8                  lane_num;
	struct kobject      kobj;
};

#define SL_CTL_LINK_MAGIC     0x536c6c6b
#define SL_CTL_LINK_VER       1
struct sl_ctl_link {
	u32                          magic;
	u32                          ver;

	u8                           num;

	struct sl_link_config        config;
	struct sl_link_policy        policy;
	spinlock_t                   config_lock;

	spinlock_t                   data_lock;
	bool                         is_canceled;

	struct sl_ctl_link_counter  *counters;

	struct {
		ktime_t              start;
		ktime_t              elapsed;
		u32                  attempt_count;
		ktime_t              attempt_start;
		ktime_t              attempt_elapsed;
		spinlock_t           lock;
	} up_clock;

	struct sl_ctl_link_fec_data  fec_data;
	struct timer_list            fec_mon_timer;
	bool                         fec_mon_timer_stop;
	spinlock_t                   fec_mon_timer_lock;
	struct work_struct           fec_mon_timer_work;
	struct sl_ctl_link_fec_cache fec_up_cache;
	struct sl_ctl_link_fec_cache fec_down_cache;
	u8                           fec_ucw_chance;
	u8                           fec_ccw_chance;

	struct sl_ctl_lgrp          *ctl_lgrp;

	struct kobject              *parent_kobj;
	struct kobject               kobj;
	struct kobject               policy_kobj;
	struct kobject               config_kobj;
	struct kobject               caps_kobj;
	struct kobject               counters_kobj;
	struct {
		struct kobject               kobj;
		struct kobject               current_kobj;
		struct kobject               current_lane_kobj;
		struct sl_ctl_link_fecl_kobj current_fecl_kobjs[SL_MAX_LANES];
		struct kobject               current_tail_kobj;
		struct kobject               up_kobj;
		struct kobject               up_lane_kobj;
		struct sl_ctl_link_fecl_kobj up_fecl_kobjs[SL_MAX_LANES];
		struct kobject               up_tail_kobj;
		struct kobject               down_kobj;
		struct kobject               down_lane_kobj;
		struct sl_ctl_link_fecl_kobj down_fecl_kobjs[SL_MAX_LANES];
		struct kobject               down_tail_kobj;
		struct kobject               mon_check_kobj;
		struct kobject               up_check_kobj;
	} fec;

	u32               state;
	struct completion down_complete;
	struct kref       ref_cnt;
	struct completion del_complete;
};

int		    sl_ctl_link_new(u8 ldev_num, u8 lgrp_num, u8 link_num, struct kobject *sysfs_parent);
int		    sl_ctl_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num);
struct sl_ctl_link *sl_ctl_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_ctl_link_config_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			   struct sl_link_config *link_config);
int sl_ctl_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			   struct sl_link_policy *link_policy);

int sl_ctl_link_an_lp_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			       struct sl_link_caps *caps, u32 timeout_ms, u32 flags);
int sl_ctl_link_an_lp_caps_stop(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_ctl_link_up(u8 ldev_num, u8 lgrp_num, u8 link_num);
int sl_ctl_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num);
int sl_ctl_link_reset(u8 ldev_num, u8 lgrp_num, u8 link_num);

void sl_ctl_link_up_clocks_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			       s64 *up_time, s64 *total_time);
void sl_ctl_link_up_count_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *up_count);

int sl_ctl_link_state_get_cmd(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *state);

int sl_ctl_link_an_lp_caps_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *state);

void sl_ctl_link_an_fail_cause_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *fail_cause, time64_t *fail_time);

u32 sl_ctl_link_an_retry_count_get(u8 ldev_num, u8 lgrp_num, u8 link_num);

#endif /* _SL_CTL_LINK_H_ */
