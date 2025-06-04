// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/err.h>

#include <linux/sl_llr.h>

#include "sl_asic.h"
#include "sl_log.h"
#include "sl_lgrp.h"
#include "sl_llr.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_llr.h"
#include "sl_sysfs.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_LLR_LOG_NAME

static struct sl_llr llrs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];

void sl_llr_init(void)
{
	u8 ldev_num;
	u8 lgrp_num;
	u8 link_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
			for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
				llrs[ldev_num][lgrp_num][link_num].magic    = SL_LLR_MAGIC;
				llrs[ldev_num][lgrp_num][link_num].ver      = SL_LLR_VER;
				llrs[ldev_num][lgrp_num][link_num].num      = link_num;
				llrs[ldev_num][lgrp_num][link_num].lgrp_num = lgrp_num;
				llrs[ldev_num][lgrp_num][link_num].ldev_num = ldev_num;
			}
		}
	}
}

static int sl_llr_check(struct sl_llr *llr)
{
	if (!llr) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL llr");
		return -EINVAL;
	}
	if (IS_ERR(llr)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "llr pointer error");
		return -EINVAL;
	}
	if (llr->magic != SL_LLR_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad llr magic");
		return -EINVAL;
	}
	if (llr->ver != SL_LLR_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "wrong llr version");
		return -EINVAL;
	}
	if (llr->num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad llr num");
		return -EINVAL;
	}
	if (llr->lgrp_num >= SL_ASIC_MAX_LGRPS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp num");
		return -EINVAL;
	}
	if (llr->ldev_num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev num");
		return -EINVAL;
	}

	return 0;
}

static int sl_llr_config_check(struct sl_llr_config *llr_config)
{
	if (!llr_config) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL llr_config");
		return -EINVAL;
	}
	if (IS_ERR(llr_config)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "llr_config pointer error");
		return -EINVAL;
	}
	if (llr_config->magic != SL_LLR_CONFIG_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad llr_config magic");
		return -EINVAL;
	}
	if (llr_config->ver != SL_LLR_CONFIG_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "wrong llr_config version");
		return -EINVAL;
	}

	return 0;
}

static int sl_llr_policy_check(struct sl_llr_policy *llr_policy)
{
	if (!llr_policy) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL llr_policy");
		return -EINVAL;
	}
	if (IS_ERR(llr_policy)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "llr_policy pointer error");
		return -EINVAL;
	}
	if (llr_policy->magic != SL_LLR_POLICY_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad llr_policy magic");
		return -EINVAL;
	}
	if (llr_policy->ver != SL_LLR_POLICY_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "wrong llr_policy version");
		return -EINVAL;
	}

	return 0;
}

struct sl_llr *sl_llr_new(struct sl_lgrp *lgrp, u8 llr_num, struct kobject *sysfs_parent)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
		return ERR_PTR(rtn);
	}
	if (llr_num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"invalid (llr_num = %u)", llr_num);
		return ERR_PTR(-EINVAL);
	}

	rtn = sl_ctl_llr_new(lgrp->ldev_num, lgrp->num, llr_num, sysfs_parent);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"ctl_llr_new failed [%d]", rtn);
		return ERR_PTR(-EINVAL);
	}

	return &llrs[lgrp->ldev_num][lgrp->num][llr_num];
}
EXPORT_SYMBOL(sl_llr_new);

int sl_llr_del(struct sl_llr *llr)
{
	int rtn;

	rtn = sl_llr_check(llr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "del fail");
		return rtn;
	}

	sl_ctl_llr_del(llr->ldev_num, llr->lgrp_num, llr->num);

	return 0;
}
EXPORT_SYMBOL(sl_llr_del);

int sl_llr_config_set(struct sl_llr *llr, struct sl_llr_config *llr_config)
{
	int rtn;

	rtn = sl_llr_check(llr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "config set fail");
		return rtn;
	}
	rtn = sl_llr_config_check(llr_config);
	if (rtn) {
		sl_log_err(llr, LOG_BLOCK, LOG_NAME, "config set fail");
		return rtn;
	}

	return sl_ctl_llr_config_set(llr->ldev_num, llr->lgrp_num, llr->num, llr_config);
}
EXPORT_SYMBOL(sl_llr_config_set);

int sl_llr_policy_set(struct sl_llr *llr, struct sl_llr_policy *llr_policy)
{
	int rtn;

	rtn = sl_llr_check(llr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "policy set fail");
		return rtn;
	}
	rtn = sl_llr_policy_check(llr_policy);
	if (rtn) {
		sl_log_err(llr, LOG_BLOCK, LOG_NAME, "policy set fail");
		return rtn;
	}

	return sl_ctl_llr_policy_set(llr->ldev_num, llr->lgrp_num, llr->num, llr_policy);
}
EXPORT_SYMBOL(sl_llr_policy_set);

int sl_llr_setup(struct sl_llr *llr)
{
	int rtn;

	rtn = sl_llr_check(llr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "setup fail");
		return rtn;
	}

	return sl_ctl_llr_setup(llr->ldev_num, llr->lgrp_num, llr->num);
}
EXPORT_SYMBOL(sl_llr_setup);

int sl_llr_start(struct sl_llr *llr)
{
	int rtn;

	rtn = sl_llr_check(llr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "start fail");
		return rtn;
	}

	return sl_ctl_llr_start(llr->ldev_num, llr->lgrp_num, llr->num);
}
EXPORT_SYMBOL(sl_llr_start);

int sl_llr_stop(struct sl_llr *llr)
{
	int rtn;

	rtn = sl_llr_check(llr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "stop fail");
		return rtn;
	}

	return sl_ctl_llr_stop(llr->ldev_num, llr->lgrp_num, llr->num);
}
EXPORT_SYMBOL(sl_llr_stop);

int sl_llr_state_get(struct sl_llr *llr, u32 *state)
{
	int rtn;

	rtn = sl_llr_check(llr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "state get fail");
		return rtn;
	}

	return sl_ctl_llr_state_get(llr->ldev_num, llr->lgrp_num, llr->num, state);
}
EXPORT_SYMBOL(sl_llr_state_get);

const char *sl_llr_state_str(u32 state)
{
	switch (state) {
	case SL_LLR_STATE_OFF:
		return "off";
	case SL_LLR_STATE_CONFIGURED:
		return "configured";
	case SL_LLR_STATE_SETUP_BUSY:
		return "setup-busy";
	case SL_LLR_STATE_SETUP:
		return "setup";
	case SL_LLR_STATE_START_BUSY:
		return "start-busy";
	case SL_LLR_STATE_RUNNING:
		return "running";
	case SL_LLR_STATE_CANCELING:
		return "canceling";
	case SL_LLR_STATE_STOP_BUSY:
		return "stop-busy";
	default:
		return "unknown";
	}
}
EXPORT_SYMBOL(sl_llr_state_str);

const char *sl_llr_link_dn_behavior_str(u32 behavior)
{
	switch (behavior) {
	case SL_LLR_LINK_DN_BEHAVIOR_DISCARD:
		return "discard";
	case SL_LLR_LINK_DN_BEHAVIOR_BLOCK:
		return "block";
	case SL_LLR_LINK_DN_BEHAVIOR_BEST_EFFORT:
		return "best-effort";
	default:
		return "unknown";
	}
}
EXPORT_SYMBOL(sl_llr_link_dn_behavior_str);
