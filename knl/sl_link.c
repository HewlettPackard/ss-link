// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/sl_link.h>

#include "sl_asic.h"
#include "sl_log.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "sl_core_str.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_LINK_LOG_NAME

static struct sl_link links[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];

void sl_link_init(void)
{
	u8 ldev_num;
	u8 lgrp_num;
	u8 link_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
			for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
				links[ldev_num][lgrp_num][link_num].magic    = SL_LINK_MAGIC;
				links[ldev_num][lgrp_num][link_num].ver      = SL_LINK_VER;
				links[ldev_num][lgrp_num][link_num].num      = link_num;
				links[ldev_num][lgrp_num][link_num].lgrp_num = lgrp_num;
				links[ldev_num][lgrp_num][link_num].ldev_num = ldev_num;
			}
		}
	}
}

int sl_link_check(struct sl_link *link)
{
	if (!link) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL link");
		return -EINVAL;
	}
	if (IS_ERR(link)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link pointer error");
		return -EINVAL;
	}
	if (link->magic != SL_LINK_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad link magic");
		return -EINVAL;
	}
	if (link->ver != SL_LINK_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad link version");
		return -EINVAL;
	}
	if (link->num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad link num");
		return -EINVAL;
	}
	if (link->lgrp_num >= SL_ASIC_MAX_LGRPS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp num");
		return -EINVAL;
	}
	if (link->ldev_num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev num");
		return -EINVAL;
	}

	return 0;
}

static int sl_link_config_check(struct sl_link_config *link_config)
{
	if (!link_config) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL link_config");
		return -EINVAL;
	}
	if (IS_ERR(link_config)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link_config pointer error");
		return -EINVAL;
	}
	if (link_config->magic != SL_LINK_CONFIG_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad link_config magic");
		return -EINVAL;
	}
	if (link_config->ver != SL_LINK_CONFIG_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad link_config version");
		return -EINVAL;
	}

	return 0;
}

static int sl_link_policy_check(struct sl_link_policy *link_policy)
{
	if (!link_policy) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL link_policy");
		return -EINVAL;
	}
	if (IS_ERR(link_policy)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link_policy pointer error");
		return -EINVAL;
	}
	if (link_policy->magic != SL_LINK_POLICY_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad link_policy magic");
		return -EINVAL;
	}
	if (link_policy->ver != SL_LINK_POLICY_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad link_policy version");
		return -EINVAL;
	}

	return 0;
}

struct sl_link *sl_link_new(struct sl_lgrp *lgrp, u8 link_num, struct kobject *sysfs_parent)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
		return ERR_PTR(rtn);
	}
	if (link_num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"invalid (link_num = %u)", link_num);
		return ERR_PTR(-EINVAL);
	}

	rtn = sl_ctl_link_new(lgrp->ldev_num, lgrp->num, link_num, sysfs_parent);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"ctl_link_link_new failed [%d]", rtn);
		return ERR_PTR(-EINVAL);
	}

	return &links[lgrp->ldev_num][lgrp->num][link_num];
}
EXPORT_SYMBOL(sl_link_new);

int sl_link_del(struct sl_link *link)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "del fail");
		return rtn;
	}

	sl_ctl_link_del(link->ldev_num, link->lgrp_num, link->num);

	return 0;
}
EXPORT_SYMBOL(sl_link_del);

int sl_link_config_set(struct sl_link *link, struct sl_link_config *link_config)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"config link_check set fail");
		return rtn;
	}
	rtn = sl_link_config_check(link_config);
	if (rtn) {
		sl_log_err(link, LOG_BLOCK, LOG_NAME,
			"config set link_config_check fail");
		return rtn;
	}

	return sl_ctl_link_config_set(link->ldev_num, link->lgrp_num, link->num, link_config);
}
EXPORT_SYMBOL(sl_link_config_set);


int sl_link_policy_set(struct sl_link *link, struct sl_link_policy *link_policy)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"policy set link_check fail");
		return rtn;
	}
	rtn = sl_link_policy_check(link_policy);
	if (rtn) {
		sl_log_err(link, LOG_BLOCK, LOG_NAME,
			"policy set link_policy_check fail");
		return rtn;
	}

	return sl_ctl_link_policy_set(link->ldev_num, link->lgrp_num, link->num, link_policy);
}
EXPORT_SYMBOL(sl_link_policy_set);

int sl_link_an_lp_caps_get(struct sl_link *link, struct sl_link_caps *caps, u32 timeout_ms, u32 flags)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lp caps get fail");
		return rtn;
	}
	if (!caps) {
		sl_log_err(link, LOG_BLOCK, LOG_NAME, "NULL caps");
		return -EINVAL;
	}
	if (timeout_ms == 0) {
		sl_log_err(link, LOG_BLOCK, LOG_NAME, "0 timeout");
		return -EINVAL;
	}

	return sl_ctl_link_an_lp_caps_get(link->ldev_num, link->lgrp_num, link->num, caps, timeout_ms, flags);
}
EXPORT_SYMBOL(sl_link_an_lp_caps_get);

int sl_link_an_lp_caps_stop(struct sl_link *link)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lp caps stop fail");
		return rtn;
	}

	return sl_ctl_link_an_lp_caps_stop(link->ldev_num, link->lgrp_num, link->num);
}
EXPORT_SYMBOL(sl_link_an_lp_caps_stop);

int sl_link_up(struct sl_link *link)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link up fail");
		return rtn;
	}

	return sl_ctl_link_up(link->ldev_num, link->lgrp_num, link->num);
}
EXPORT_SYMBOL(sl_link_up);

int sl_link_down(struct sl_link *link)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link down fail");
		return rtn;
	}

	return sl_ctl_link_down(link->ldev_num, link->lgrp_num, link->num);
}
EXPORT_SYMBOL(sl_link_down);

int sl_link_reset(struct sl_link *link)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link reset fail");
		return rtn;
	}

	return sl_ctl_link_reset(link->ldev_num, link->lgrp_num, link->num);
}
EXPORT_SYMBOL(sl_link_reset);

int sl_link_clocks_get(struct sl_link *link, u32 *up_count, u32 *up_time, u32 *total_time)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link clocks get fail");
		return rtn;
	}

	sl_ctl_link_up_clocks_get(link->ldev_num, link->lgrp_num, link->num, up_time, total_time);
	sl_ctl_link_up_count_get(link->ldev_num, link->lgrp_num, link->num, up_count);

	return 0;
}
EXPORT_SYMBOL(sl_link_clocks_get);

int sl_link_state_get(struct sl_link *link, u32 *state)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link state get fail");
		return rtn;
	}
	if (!state) {
		sl_log_err(link, LOG_BLOCK, LOG_NAME, "NULL state");
		return -EINVAL;
	}

	return sl_ctl_link_state_get(link->ldev_num, link->lgrp_num, link->num, state);
}
EXPORT_SYMBOL(sl_link_state_get);

const char *sl_link_state_str(u32 state)
{
	switch (state) {
	case SL_LINK_STATE_INVALID:
		return "invalid";
	case SL_LINK_STATE_ERROR:
		return "error";
	case SL_LINK_STATE_DOWN:
		return "down";
	case SL_LINK_STATE_AN:
		return "autoneg";
	case SL_LINK_STATE_STARTING:
		return "starting";
	case SL_LINK_STATE_UP:
		return "up";
	case SL_LINK_STATE_STOPPING:
		return "stopping";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_link_state_str);

const char *sl_link_config_opt_str(u32 option)
{
	switch (option) {
	case SL_LINK_CONFIG_OPT_AUTONEG_ENABLE:
		return "autoneg-enable";
	case SL_LINK_CONFIG_OPT_AUTONEG_CONTINUOUS_ENABLE:
		return "autoneg-continuous";
	case SL_LINK_CONFIG_OPT_SERDES_LOOPBACK_ENABLE:
		return "loopback-serdes-enable";
	case SL_LINK_CONFIG_OPT_HEADSHELL_LOOPBACK_ENABLE:
		return "loopback-headshell-enable";
	case SL_LINK_CONFIG_OPT_REMOTE_LOOPBACK_ENABLE:
		return "loopback-remote-enable";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_link_config_opt_str);

const char *sl_link_policy_opt_str(u32 option)
{
	switch (option) {
	case SL_LINK_POLICY_OPT_KEEP_SERDES_UP:
		return "keep-serdes-up";
	case SL_LINK_POLICY_OPT_USE_UNSUPPORTED_CABLE:
		return "use-unsupported-cable";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_link_policy_opt_str);

const char *sl_link_down_cause_str(u32 cause)
{
	switch (cause) {
	case SL_LINK_DOWN_CAUSE_INVALID:
		return "invalid";
	case SL_LINK_DOWN_CAUSE_NONE:
		return "none";
	case SL_LINK_DOWN_CAUSE_BAD_EYE:
		return "bad-eye";
	case SL_LINK_DOWN_CAUSE_UCW:
		return "ucw";
	case SL_LINK_DOWN_CAUSE_CCW:
		return "ccw";
	case SL_LINK_DOWN_CAUSE_ALIGN:
		return "alignment";
	case SL_LINK_DOWN_CAUSE_LF:
		return "lcl-fault";
	case SL_LINK_DOWN_CAUSE_RF:
		return "rmt-fault";
	case SL_LINK_DOWN_CAUSE_SERDES:
		return "serdes";
	case SL_LINK_DOWN_CAUSE_DOWN:
		return "down";
	case SL_LINK_DOWN_CAUSE_UP_TRIES:
		return "up-tries";
	case SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH:
		return "autoneg-nomatch";
	case SL_LINK_DOWN_CAUSE_AUTONEG_FAIL:
		return "autoneg-fail";
	case SL_LINK_DOWN_CAUSE_CONFIG:
		return "config";
	case SL_LINK_DOWN_CAUSE_INTR_ENABLE:
		return "intr-enable";
	case SL_LINK_DOWN_CAUSE_TIMEOUT:
		return "timeout";
	case SL_LINK_DOWN_CAUSE_CANCELED:
		return "canceled";
	case SL_LINK_DOWN_CAUSE_LLR_STARVED:
		return "llr-starved";
	case SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE:
		return "unsupported-cable";
	case SL_LINK_DOWN_CAUSE_COMMAND:
		return "command";
	case SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX:
		return "llr-replay-at-max";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_link_down_cause_str);

int sl_link_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size)
{
	if (!info_map_str) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL info_map_str");
		return -EINVAL;
	}

	sl_core_info_map_str(info_map, info_map_str, info_map_str_size);

	return 0;
}
EXPORT_SYMBOL(sl_link_info_map_str);

const char *sl_link_config_pause_str(u32 config)
{
	if (config == SL_LINK_CONFIG_PAUSE_ASYM)
		return "pause-asym";
	if (config == SL_LINK_CONFIG_PAUSE_SYM)
		return "pause-sym";

	return "unrecognized";
}
EXPORT_SYMBOL(sl_link_config_pause_str);

const char *sl_link_config_hpe_str(u32 config)
{
	if (config == SL_LINK_CONFIG_HPE_LINKTRAIN)
		return "linktrain";
	if (config == SL_LINK_CONFIG_HPE_PRECODING)
		return "precode";
	if (config == SL_LINK_CONFIG_HPE_PCAL)
		return "progressive-cal";
	if (config == SL_LINK_CONFIG_HPE_R3)
		return "R3";
	if (config == SL_LINK_CONFIG_HPE_R2)
		return "R2";
	if (config == SL_LINK_CONFIG_HPE_R1)
		return "R1";
	if (config == SL_LINK_CONFIG_HPE_C3)
		return "C3";
	if (config == SL_LINK_CONFIG_HPE_C2)
		return "C2";
	if (config == SL_LINK_CONFIG_HPE_C1)
		return "C1";
	if (config == SL_LINK_CONFIG_HPE_LLR)
		return "llr";

	return "unrecognized";
}
EXPORT_SYMBOL(sl_link_config_hpe_str);
