/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LINK_H_
#define _SL_CTRL_LINK_H_

#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>

#include <linux/hpe/sl/sl_link.h>
#include <linux/hpe/sl/sl_lgrp.h>

#include "sl_ctrl_link_fec_priv.h"

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

struct sl_ctrl_link_fecl_kobj {
	struct sl_ctrl_link *ctrl_link;
	u8                   lane_num;
	struct kobject       kobj;
};

#define SL_CTRL_LINK_MAGIC     0x536c6c6b
#define SL_CTRL_LINK_VER       1
struct sl_ctrl_link {
	u32                          magic;
	u32                          ver;

	u8                           num;

	struct sl_link_config        config;
	struct sl_link_policy        policy;
	spinlock_t                   config_lock;

	spinlock_t                   data_lock;
	bool                         is_canceled;

	struct sl_ctrl_link_counter *counters;
	struct sl_ctrl_link_counter *cause_counters;
	struct sl_ctrl_link_counter *an_cause_counters;

	struct {
		ktime_t              start;
		ktime_t              elapsed;
		u32                  attempt_count;
		ktime_t              attempt_start;
		ktime_t              attempt_elapsed;
		ktime_t              up;
		spinlock_t           lock;
	} up_clock;

	struct sl_ctrl_link_fec_data  fec_data;
	u32                           fec_mon_state;
	struct timer_list             fec_mon_timer;
	bool                          fec_mon_timer_stop;
	spinlock_t                    fec_mon_timer_lock;
	struct work_struct            fec_mon_timer_work;
	struct sl_ctrl_link_fec_cache fec_up_cache;
	struct sl_ctrl_link_fec_cache fec_down_cache;
	u8                            fec_ucw_chance;
	u8                            fec_ccw_chance;

	struct sl_ctrl_lgrp         *ctrl_lgrp;

	struct kobject              *parent_kobj;
	struct kobject               kobj;
	struct kobject               policy_kobj;
	struct kobject               config_kobj;
	struct kobject               caps_kobj;
	struct kobject               counters_kobj;
	struct {
		struct kobject                kobj;

		struct kobject                mon_check_kobj;

		struct kobject                down_kobj;
		struct kobject                down_lane_kobj;
		struct sl_ctrl_link_fecl_kobj down_fecl_kobjs[SL_MAX_LANES];
		struct kobject                down_tail_kobj;

		struct kobject                up_kobj;
		struct kobject                up_lane_kobj;
		struct sl_ctrl_link_fecl_kobj up_fecl_kobjs[SL_MAX_LANES];
		struct kobject                up_tail_kobj;
	} fec;

	u32               state;
	struct completion down_complete;
	struct kref       ref_cnt;
	struct completion del_complete;
	u64               last_up_fail_cause_map;
	time64_t          last_up_fail_time;
};

int		     sl_ctrl_link_new(u8 ldev_num, u8 lgrp_num, u8 link_num, struct kobject *sysfs_parent);
int                  sl_ctrl_link_put(struct sl_ctrl_link *ctrl_link);
int		     sl_ctrl_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num);
struct sl_ctrl_link *sl_ctrl_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num);
bool                 sl_ctrl_link_kref_get_unless_zero(struct sl_ctrl_link *ctrl_link);
struct sl_ctrl_link *sl_ctrl_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_ctrl_link_config_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			    struct sl_link_config *link_config);
int sl_ctrl_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			    struct sl_link_policy *link_policy);

int sl_ctrl_link_an_lp_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				struct sl_link_caps *caps, u32 timeout_ms, u32 flags);
int sl_ctrl_link_an_lp_caps_stop(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_ctrl_link_up(u8 ldev_num, u8 lgrp_num, u8 link_num);
int sl_ctrl_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num);
int sl_ctrl_link_reset(u8 ldev_num, u8 lgrp_num, u8 link_num);

void sl_ctrl_link_up_clocks_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				s64 *attempt_time, s64 *total_time, s64 *up_time);
void sl_ctrl_link_up_count_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *up_count);

int sl_ctrl_link_state_get_cmd(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *state);

int sl_ctrl_link_an_lp_caps_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *state);

void sl_ctrl_link_an_fail_cause_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *fail_cause, time64_t *fail_time);

u32 sl_ctrl_link_an_retry_count_get(struct sl_ctrl_link *ctrl_link, int *count);

int sl_ctrl_link_info_map_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u64 *info_map);

#endif /* _SL_CTRL_LINK_H_ */
