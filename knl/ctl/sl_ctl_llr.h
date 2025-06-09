/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LLR_H_
#define _SL_CTL_LLR_H_

#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>

#include <linux/sl_llr.h>

#define SL_LLR_DATA_MAGIC 0x636c6c72
#define SL_LLR_DATA_VER   2

#define SL_CTL_LLR_MAGIC 0x606c6c72
#define SL_CTL_LLR_VER   1
struct sl_ctl_llr {
	u32                         magic;
	u32                         ver;

	u8                          num;

	spinlock_t                  data_lock;

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

	struct sl_ctl_lgrp         *ctl_lgrp;

	struct kobject             *parent_kobj;
	struct kobject              kobj;
	struct kobject              config_kobj;
	struct kobject              policy_kobj;
	struct kobject              loop_time_kobj;

	struct kref                 ref_cnt;
	struct completion           del_complete;
};

int		   sl_ctl_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct kobject *sysfs_parent);
int		   sl_ctl_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num);
struct sl_ctl_llr *sl_ctl_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num);

int  sl_ctl_llr_config_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_config *llr_config);
int  sl_ctl_llr_policy_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy);

int  sl_ctl_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num);
int  sl_ctl_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num);
int  sl_ctl_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num);

int  sl_ctl_llr_state_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *state);

#endif /* _SL_CTL_LLR_H_ */
