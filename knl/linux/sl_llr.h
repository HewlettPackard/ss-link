/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_LLR_H_
#define _LINUX_SL_LLR_H_

#include "uapi/sl_llr.h"

struct sl_lgrp;
struct sl_llr;
struct sl_llr_config;
struct sl_llr_policy;
struct kobject;

struct sl_llr_data {
	u32 magic;
	u32 ver;
	u32 size;

	struct {
		u64 calculated;
		u64 min;
		u64 max;
		u64 average;
	} loop;
};

struct sl_llr *sl_llr_new(struct sl_lgrp *lgrp, u8 llr_num, struct kobject *sysfs_parent);
int            sl_llr_del(struct sl_llr *llr);

int sl_llr_config_set(struct sl_llr *llr, struct sl_llr_config *llr_config);
int sl_llr_policy_set(struct sl_llr *llr, struct sl_llr_policy *llr_policy);
int sl_llr_start(struct sl_llr *llr);
int sl_llr_stop(struct sl_llr *llr);
int sl_llr_state_get(struct sl_llr *llr, u32 *state);

const char *sl_llr_state_str(u32 state);
const char *sl_llr_link_dn_behavior_str(u32 behavior);

#endif /* _LINUX_SL_LLR_H_ */
