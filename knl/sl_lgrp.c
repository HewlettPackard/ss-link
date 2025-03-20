// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/sl_lgrp.h>

#include "sl_asic.h"
#include "sl_log.h"
#include "sl_ldev.h"
#include "sl_lgrp.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_LGRP_LOG_NAME

static struct sl_lgrp lgrps[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS];

void sl_lgrp_init(void)
{
	u8 ldev_num;
	u8 lgrp_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
			lgrps[ldev_num][lgrp_num].magic    = SL_LGRP_MAGIC;
			lgrps[ldev_num][lgrp_num].ver      = SL_LGRP_VER;
			lgrps[ldev_num][lgrp_num].num      = lgrp_num;
			lgrps[ldev_num][lgrp_num].ldev_num = ldev_num;
		}
	}
}

int sl_lgrp_check(struct sl_lgrp *lgrp)
{
	if (!lgrp) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL lgrp");
		return -EINVAL;
	}
	if (IS_ERR(lgrp)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp pointer error");
		return -EINVAL;
	}
	if (lgrp->magic != SL_LGRP_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp magic");
		return -EINVAL;
	}
	if (lgrp->ver != SL_LGRP_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp version");
		return -EINVAL;
	}
	if (lgrp->num >= SL_ASIC_MAX_LGRPS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp num");
		return -EINVAL;
	}
	if (lgrp->ldev_num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev num");
		return -EINVAL;
	}

	return 0;
}

static int sl_lgrp_config_check(struct sl_lgrp_config *lgrp_config)
{
	if (!lgrp_config) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL lgrp_config");
		return -EINVAL;
	}
	if (IS_ERR(lgrp_config)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_config pointer error");
		return -EINVAL;
	}
	if (lgrp_config->magic != SL_LGRP_CONFIG_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp_config magic");
		return -EINVAL;
	}
	if (lgrp_config->ver != SL_LGRP_CONFIG_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp_config version");
		return -EINVAL;
	}

	return 0;
}

static int sl_lgrp_policy_check(struct sl_lgrp_policy *lgrp_policy)
{
	if (!lgrp_policy) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL lgrp_policy");
		return -EINVAL;
	}
	if (IS_ERR(lgrp_policy)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_policy pointer error");
		return -EINVAL;
	}
	if (lgrp_policy->magic != SL_LGRP_POLICY_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp_policy magic");
		return -EINVAL;
	}
	if (lgrp_policy->ver != SL_LGRP_POLICY_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp_policy version");
		return -EINVAL;
	}

	return 0;
}

int sl_lgrp_hw_attr_set(struct sl_lgrp *lgrp, struct sl_hw_attr *hw_attr)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "hw attr set fail");
		return rtn;
	}
	if (!hw_attr) {
		sl_log_err(lgrp, LOG_BLOCK, LOG_NAME, "NULL hw_attr");
		return -EINVAL;
	}

	rtn = sl_core_lgrp_hw_attr_set(lgrp->ldev_num, lgrp->num, hw_attr);
	if (rtn) {
		sl_log_err(lgrp, LOG_BLOCK, LOG_NAME, "core_lgrp_hw_attr_set failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
EXPORT_SYMBOL(sl_lgrp_hw_attr_set);

struct sl_lgrp *sl_lgrp_new(struct sl_ldev *ldev, u8 lgrp_num, struct kobject *sysfs_parent)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
		return ERR_PTR(rtn);
	}
	if (lgrp_num >= SL_ASIC_MAX_LGRPS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"invalid (lgrp_num = %u)", lgrp_num);
		return ERR_PTR(-EINVAL);
	}

	rtn = sl_ctl_lgrp_new(ldev->num, lgrp_num, sysfs_parent);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"ctl_lgrp_new failed [%d]", rtn);
		return ERR_PTR(-EINVAL);
	}

	return &lgrps[ldev->num][lgrp_num];
}
EXPORT_SYMBOL(sl_lgrp_new);

int sl_lgrp_del(struct sl_lgrp *lgrp)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "del fail");
		return rtn;
	}

	sl_ctl_lgrp_del(lgrp->ldev_num, lgrp->num);

	return 0;
}
EXPORT_SYMBOL(sl_lgrp_del);

int sl_lgrp_config_set(struct sl_lgrp *lgrp, struct sl_lgrp_config *lgrp_config)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "config set check fail");
		return rtn;
	}
	rtn = sl_lgrp_config_check(lgrp_config);
	if (rtn) {
		sl_log_err(lgrp, LOG_BLOCK, LOG_NAME, "config set config check fail");
		return rtn;
	}

	return sl_ctl_lgrp_config_set(lgrp->ldev_num, lgrp->num, lgrp_config);
}
EXPORT_SYMBOL(sl_lgrp_config_set);

int sl_lgrp_policy_set(struct sl_lgrp *lgrp, struct sl_lgrp_policy *lgrp_policy)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "policy set fail");
		return rtn;
	}
	rtn = sl_lgrp_policy_check(lgrp_policy);
	if (rtn) {
		sl_log_err(lgrp, LOG_BLOCK, LOG_NAME, "policy set fail");
		return rtn;
	}

	return sl_ctl_lgrp_policy_set(lgrp->ldev_num, lgrp->num, lgrp_policy);
}
EXPORT_SYMBOL(sl_lgrp_policy_set);

int sl_lgrp_notif_callback_reg(struct sl_lgrp *lgrp, sl_lgrp_notif_t callback,
	u32 types, void *tag)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "callback reg fail");
		return rtn;
	}
	if (!callback) {
		sl_log_err(lgrp, LOG_BLOCK, LOG_NAME, "NULL callback");
		return -EINVAL;
	}

	return sl_ctl_lgrp_notif_callback_reg(lgrp->ldev_num, lgrp->num, callback, types, tag);
}
EXPORT_SYMBOL(sl_lgrp_notif_callback_reg);

int sl_lgrp_notif_callback_unreg(struct sl_lgrp *lgrp, sl_lgrp_notif_t callback,
	u32 types)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "callback unreg fail");
		return rtn;
	}
	if (!callback) {
		sl_log_err(lgrp, LOG_BLOCK, LOG_NAME, "NULL callback");
		return -EINVAL;
	}

	return sl_ctl_lgrp_notif_callback_unreg(lgrp->ldev_num, lgrp->num, callback, types);
}
EXPORT_SYMBOL(sl_lgrp_notif_callback_unreg);

int sl_lgrp_connect_id_set(struct sl_lgrp *lgrp, const char *connect_id)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "id set fail");
		return rtn;
	}
	if (!connect_id) {
		sl_log_err(lgrp, LOG_BLOCK, LOG_NAME, "NULL connect_id");
		return -EINVAL;
	}

	return sl_ctl_lgrp_connect_id_set(lgrp->ldev_num, lgrp->num, connect_id);
}
EXPORT_SYMBOL(sl_lgrp_connect_id_set);

const char *sl_lgrp_config_tech_str(u32 config)
{
	if (config == SL_LGRP_CONFIG_TECH_CK_400G)
		return "ck400G";
	if (config == SL_LGRP_CONFIG_TECH_CK_200G)
		return "ck200G";
	if (config == SL_LGRP_CONFIG_TECH_CK_100G)
		return "ck100G";
	if (config == SL_LGRP_CONFIG_TECH_BS_200G)
		return "bs200G";
	if (config == SL_LGRP_CONFIG_TECH_CD_100G)
		return "cd100G";
	if (config == SL_LGRP_CONFIG_TECH_CD_50G)
		return "cd50G";
	if (config == SL_LGRP_CONFIG_TECH_BJ_100G)
		return "bj100G";

	return "unrecognized";
}
EXPORT_SYMBOL(sl_lgrp_config_tech_str);

const char *sl_lgrp_config_fec_str(u32 config)
{
	if (config == SL_LGRP_CONFIG_FEC_RS_LL)
		return "reed-soloman-low-latency";
	if (config == SL_LGRP_CONFIG_FEC_RS)
		return "reed-soloman";

	return "unrecognized";
}
EXPORT_SYMBOL(sl_lgrp_config_fec_str);

const char *sl_lgrp_furcation_str(u32 furcation)
{
	if (furcation == SL_MEDIA_FURCATION_X1)
		return "unfurcated";
	if (furcation == SL_MEDIA_FURCATION_X2)
		return "bifurcated";
	if (furcation == SL_MEDIA_FURCATION_X4)
		return "quadfurcated";

	return "unrecognized";
}
EXPORT_SYMBOL(sl_lgrp_furcation_str);

const char *sl_lgrp_notif_str(u32 notif)
{
	switch (notif) {
	case SL_LGRP_NOTIF_INVALID:
		return "invalid";
	case SL_LGRP_NOTIF_LINK_UP:
		return "link-up";
	case SL_LGRP_NOTIF_LINK_UP_FAIL:
		return "link-up-fail";
	case SL_LGRP_NOTIF_LINK_ASYNC_DOWN:
		return "link-async-down";
	case SL_LGRP_NOTIF_LINK_ERROR:
		return "link-error";
	case SL_LGRP_NOTIF_LLR_DATA:
		return "llr-data";
	case SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT:
		return "llr-setup-timeout";
	case SL_LGRP_NOTIF_LLR_START_TIMEOUT:
		return "llr-start-timeout";
	case SL_LGRP_NOTIF_LLR_RUNNING:
		return "llr-running";
	case SL_LGRP_NOTIF_LLR_ERROR:
		return "llr-error";
	case SL_LGRP_NOTIF_MEDIA_PRESENT:
		return "media-present";
	case SL_LGRP_NOTIF_MEDIA_NOT_PRESENT:
		return "media-not-present";
	case SL_LGRP_NOTIF_MEDIA_ERROR:
		return "media-error";
	case SL_LGRP_NOTIF_LINK_UCW_WARN:
		return "link-ucw-warn";
	case SL_LGRP_NOTIF_LINK_CCW_WARN:
		return "link-ccw-warn";
	case SL_LGRP_NOTIF_AN_DATA:
		return "an-data";
	case SL_LGRP_NOTIF_AN_TIMEOUT:
		return "an-timeout";
	case SL_LGRP_NOTIF_AN_ERROR:
		return "an-error";
	case SL_LGRP_NOTIF_LLR_CANCELED:
		return "llr-canceled";
	case SL_LGRP_NOTIF_LINK_DOWN:
		return "link-down";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_lgrp_notif_str);

const char *sl_lgrp_config_opt_str(u32 option)
{
	switch (option) {
	case SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE:
		return "loopback-serdes-enable";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_lgrp_config_opt_str);

const char *sl_lgrp_fec_mode_str(u32 mode)
{
	switch (mode) {
	case SL_LGRP_FEC_MODE_OFF:
		return "off";
	case SL_LGRP_FEC_MODE_MONITOR:
		return "monitor";
	case SL_LGRP_FEC_MODE_CORRECT:
		return "correct";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_lgrp_fec_mode_str);

const char *sl_lgrp_llr_mode_str(u32 mode)
{
	switch (mode) {
	case SL_LGRP_LLR_MODE_OFF:
		return "off";
	case SL_LGRP_LLR_MODE_MONITOR:
		return "monitor";
	case SL_LGRP_LLR_MODE_ON:
		return "on";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_lgrp_llr_mode_str);
