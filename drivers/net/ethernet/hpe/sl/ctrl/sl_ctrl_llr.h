/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LLR_H_
#define _SL_CTRL_LLR_H_

#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>

#include <linux/hpe/sl/sl_llr.h>

#define SL_LLR_DATA_MAGIC 0x636c6c72
#define SL_LLR_DATA_VER   2

#define SL_CTRL_LLR_MAGIC 0x606c6c72
#define SL_CTRL_LLR_VER   1
struct sl_ctrl_llr {
	u32                         magic;
	u32                         ver;

	u8                          num;

	spinlock_t                  data_lock;

	struct sl_ctrl_llr_counter *counters;

	struct sl_llr_config        config;
	struct sl_llr_policy        policy;
	struct {
		u32                 state;
		u64                 imap;
		struct sl_llr_data  data;
	} setup;
	struct {
		u32                 state;
		u64                 imap;
	} start;

	struct sl_ctrl_lgrp        *ctrl_lgrp;

	struct kobject             *parent_kobj;
	struct kobject              config_kobj;
	struct kobject              counters_kobj;

	struct kref                 ref_cnt;
	struct completion           del_complete;
};

int		    sl_ctrl_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct kobject *sysfs_parent);
int		    sl_ctrl_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num);
struct sl_ctrl_llr *sl_ctrl_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num);

int sl_ctrl_llr_config_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_config *llr_config);
int sl_ctrl_llr_policy_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy);

int sl_ctrl_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num);
int sl_ctrl_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num);
int sl_ctrl_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num);

u32 sl_ctrl_llr_state_from_core_llr_state(u32 core_llr_state);
int sl_ctrl_llr_state_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *state);

int sl_ctrl_llr_info_map_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u64 *info_map);

#endif /* _SL_CTRL_LLR_H_ */
