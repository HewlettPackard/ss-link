/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_LLR_H_
#define _LINUX_SL_LLR_H_

#include <linux/bitops.h>
#include <linux/kobject.h>

struct sl_lgrp;
struct sl_llr;

#define SL_LLR_LINK_DN_BEHAVIOR_DISCARD     BIT(0)
#define SL_LLR_LINK_DN_BEHAVIOR_BLOCK       BIT(1)
#define SL_LLR_LINK_DN_BEHAVIOR_BEST_EFFORT BIT(2)

#define SL_LLR_CONFIG_MAGIC 0x636c6c72
#define SL_LLR_CONFIG_VER   1
struct sl_llr_config {
	u32 magic;
	u32 ver;
	u32 size;

	u32 mode;
	u32 setup_timeout_ms;
	u32 start_timeout_ms;
	u32 link_dn_behavior;

	u32 options;
};

#define SL_LLR_POLICY_OPT_CONTINUOUS_START_TRIES BIT(0)

#define SL_LLR_POLICY_MAGIC 0x636c6c72
#define SL_LLR_POLICY_VER   1
struct sl_llr_policy {
	u32 magic;
	u32 ver;
	u32 size;

	u32 options;
};

enum sl_llr_state {
	SL_LLR_STATE_INVALID     = 0,
	SL_LLR_STATE_OFF,
	SL_LLR_STATE_CONFIGURED,
	SL_LLR_STATE_SETUP_BUSY,
	SL_LLR_STATE_SETUP,
	SL_LLR_STATE_START_BUSY,
	SL_LLR_STATE_RUNNING,
	SL_LLR_STATE_CANCELING,
	SL_LLR_STATE_STOP_BUSY,
};

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

int sl_llr_setup(struct sl_llr *llr);
int sl_llr_start(struct sl_llr *llr);
int sl_llr_stop(struct sl_llr *llr);

int sl_llr_state_get(struct sl_llr *llr, u32 *state);

const char *sl_llr_state_str(u32 state);
const char *sl_llr_link_dn_behavior_str(u32 behavior);

#endif /* _LINUX_SL_LLR_H_ */
