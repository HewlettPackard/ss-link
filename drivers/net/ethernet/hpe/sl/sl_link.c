// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023-2026 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/err.h>

#include <linux/hpe/sl/sl_link.h>

#include "sl_asic.h"
#include "sl_log.h"
#include "sl_common.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "sl_core_str.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"

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
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "invalid (link_num = %u)", link_num);
		return ERR_PTR(-EINVAL);
	}

	rtn = sl_ctrl_link_new(lgrp->ldev_num, lgrp->num, link_num, sysfs_parent);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
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

	return sl_ctrl_link_del(link->ldev_num, link->lgrp_num, link->num);
}
EXPORT_SYMBOL(sl_link_del);

int sl_link_config_set(struct sl_link *link, struct sl_link_config *link_config)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link config set fail");
		return rtn;
	}
	rtn = sl_link_config_check(link_config);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link config set fail");
		return rtn;
	}

	return sl_ctrl_link_config_set(link->ldev_num, link->lgrp_num, link->num, link_config);
}
EXPORT_SYMBOL(sl_link_config_set);


int sl_link_policy_set(struct sl_link *link, struct sl_link_policy *link_policy)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "policy set fail");
		return rtn;
	}
	rtn = sl_link_policy_check(link_policy);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "policy set fail");
		return rtn;
	}

	return sl_ctrl_link_policy_set(link->ldev_num, link->lgrp_num, link->num, link_policy);
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
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL caps");
		return -EINVAL;
	}
	if (timeout_ms == 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "0 timeout");
		return -EINVAL;
	}

	return sl_ctrl_link_an_lp_caps_get(link->ldev_num, link->lgrp_num, link->num, caps, timeout_ms, flags);
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

	return sl_ctrl_link_an_lp_caps_stop(link->ldev_num, link->lgrp_num, link->num);
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

	return sl_ctrl_link_up(link->ldev_num, link->lgrp_num, link->num);
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

	return sl_ctrl_link_down(link->ldev_num, link->lgrp_num, link->num);
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

	return sl_ctrl_link_reset(link->ldev_num, link->lgrp_num, link->num);
}
EXPORT_SYMBOL(sl_link_reset);

int sl_link_clocks_get(struct sl_link *link, u32 *up_count, s64 *up_time, s64 *total_time)
{
	int rtn;
	s64 up;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link clocks get fail");
		return rtn;
	}

	rtn = sl_ctrl_link_up_time_get(link->ldev_num, link->lgrp_num, link->num, up_time, total_time, &up);
	if (rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link_up_time_get failed [%d]", rtn);

	rtn = sl_ctrl_link_up_count_get(link->ldev_num, link->lgrp_num, link->num, up_count);
	if (rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "link_up_count_get failed [%d]", rtn);

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
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL state");
		return -EINVAL;
	}

	return sl_ctrl_link_state_get_cmd(link->ldev_num, link->lgrp_num, link->num, state);
}
EXPORT_SYMBOL(sl_link_state_get);

const char *sl_link_state_str(u32 state)
{
	switch (state) {
	case SL_LINK_STATE_INVALID:
		return "invalid";
	case SL_LINK_STATE_DOWN:
		return "down";
	case SL_LINK_STATE_AN:
		return "autoneg";
	case SL_LINK_STATE_STARTING:
		return "starting";
	case SL_LINK_STATE_UP:
		return "up";
	case SL_LINK_STATE_UP_DOWN_REQ:
		return "up-requested-down";
	case SL_LINK_STATE_STOPPING:
		return "stopping";
	default:
		return "unknown";
	}
}
EXPORT_SYMBOL(sl_link_state_str);

const char *sl_link_degrade_state_str(u32 degrade_state)
{
	switch (degrade_state) {
	case SL_LINK_DEGRADE_STATE_ACTIVE:
		return "active";
	case SL_LINK_DEGRADE_STATE_INACTIVE:
		return "inactive";
	case SL_LINK_DEGRADE_STATE_FAILED:
		return "error";
	case SL_LINK_DEGRADE_STATE_TURNED_OFF:
		return "decided-off";
	case SL_LINK_DEGRADE_STATE_INVALID:
		return "invalid";
	default:
		return "unknown";
	}
}
EXPORT_SYMBOL(sl_link_degrade_state_str);

const char *sl_link_an_lp_caps_state_str(u32 lp_caps_state)
{
	switch (lp_caps_state) {
	case SL_CORE_LINK_LP_CAPS_NOT_RUNNING:
		return "not_running";
	case SL_CORE_LINK_LP_CAPS_RUNNING:
		return "running";
	case SL_CORE_LINK_LP_CAPS_DATA:
		return "data";
	case SL_CORE_LINK_LP_CAPS_TIMEOUT:
		return "timeout";
	case SL_CORE_LINK_LP_CAPS_ERROR:
		return "error";
	default:
		return "unknown";
	}
}
EXPORT_SYMBOL(sl_link_an_lp_caps_state_str);

const char *sl_link_config_opt_str(u32 option)
{
	switch (option) {
	case SL_LINK_CONFIG_OPT_AUTONEG_ENABLE:
		return "autoneg-enable";
	case SL_LINK_CONFIG_OPT_AUTONEG_CONTINUOUS_ENABLE:
		return "autoneg-continuous";
	default:
		return "unknown";
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
	case SL_LINK_POLICY_OPT_IGNORE_MEDIA_ERROR:
		return "ignore-media-error";
	case SL_LINK_POLICY_OPT_USE_SUPPORTED_SS200_CABLE:
		return "use-supported-ss200-cable";
	case SL_LINK_POLICY_OPT_LINK_DOWN_REQ:
		return "link-down-request";
	default:
		return "unknown";
	}
}
EXPORT_SYMBOL(sl_link_policy_opt_str);

#define SL_LINK_DOWN_INFO_MASK (     \
	SL_LINK_DOWN_RETRYABLE      |\
	SL_LINK_DOWN_ORIGIN_ASYNC   |\
	SL_LINK_DOWN_ORIGIN_LINK_UP)

static const char * const sl_link_down_cause_strings[] = {
	[SL_LINK_DOWN_CAUSE_UCW_BIT]                        = "ucw",
	[SL_LINK_DOWN_CAUSE_LF_BIT]                         = "lcl-fault",
	[SL_LINK_DOWN_CAUSE_RF_BIT]                         = "rmt-fault",
	[SL_LINK_DOWN_CAUSE_DOWN_BIT]                       = "link-down",
	[SL_LINK_DOWN_CAUSE_UP_TRIES_BIT]                   = "up-tries",
	[SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH_BIT]            = "autoneg-nomatch",
	[SL_LINK_DOWN_CAUSE_AUTONEG_BIT]                    = "autoneg-fail",
	[SL_LINK_DOWN_CAUSE_CONFIG_BIT]                     = "config",
	[SL_LINK_DOWN_CAUSE_INTR_UP_ENABLE_BIT]             = "intr-up-enable",
	[SL_LINK_DOWN_CAUSE_INTR_HIGH_SER_ENABLE_BIT]       = "intr-high-ser-enable",
	[SL_LINK_DOWN_CAUSE_INTR_LLR_STARVED_ENABLE_BIT]    = "intr-llr-starved-enable",
	[SL_LINK_DOWN_CAUSE_INTR_LLR_MAX_STARVE_ENABLE_BIT] = "intr-llr-max-starve-enable",
	[SL_LINK_DOWN_CAUSE_INTR_FAULT_ENABLE_BIT]          = "intr-fault-enable",
	[SL_LINK_DOWN_CAUSE_INTR_LANE_DEGRADE_ENABLE_BIT]   = "intr-lane-degrade-enable",
	[SL_LINK_DOWN_CAUSE_TIMEOUT_BIT]                    = "timeout",
	[SL_LINK_DOWN_CAUSE_CANCELED_BIT]                   = "canceled",
	[SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE_BIT]          = "unsupported-cable",
	[SL_LINK_DOWN_CAUSE_COMMAND_BIT]                    = "command",
	[SL_LINK_DOWN_CAUSE_DOWNSHIFT_BIT]                  = "downshift-fail",
	[SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX_BIT]             = "llr-replay-at-max",
	[SL_LINK_DOWN_CAUSE_UPSHIFT_BIT]                    = "upshift-fail",
	[SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG_BIT]             = "an-config",
	[SL_LINK_DOWN_CAUSE_PCS_FAULT_BIT]                  = "pcs-fault",
	[SL_LINK_DOWN_CAUSE_SERDES_PLL_BIT]                 = "serdes-pll",
	[SL_LINK_DOWN_CAUSE_SERDES_CONFIG_BIT]              = "serdes-config",
	[SL_LINK_DOWN_CAUSE_SERDES_SIGNAL_BIT]              = "serdes-signal",
	[SL_LINK_DOWN_CAUSE_SERDES_QUALITY_BIT]             = "serdes-quality",
	[SL_LINK_DOWN_CAUSE_NO_MEDIA_BIT]                   = "no-media",
	[SL_LINK_DOWN_CAUSE_CCW_BIT]                        = "ccw",
	[SL_LINK_DOWN_CAUSE_MEDIA_HOT_BIT]                  = "media-hot",
	[SL_LINK_DOWN_CAUSE_MEDIA_WARM_BIT]                 = "media-warm",
	[SL_LINK_DOWN_CAUSE_INTR_REGISTER_BIT]              = "interrupt-reg",
	[SL_LINK_DOWN_CAUSE_MEDIA_ERROR_BIT]                = "media-error",
	[SL_LINK_DOWN_CAUSE_UP_CANCELED_BIT]                = "up-canceled",
	[SL_LINK_DOWN_CAUSE_UNSUPPORTED_SPEED_BIT]          = "unsupported-speed",
	[SL_LINK_DOWN_CAUSE_SS200_CABLE_BIT]                = "ss200-cable",
	[SL_LINK_DOWN_CAUSE_TX_LOL_BIT]                     = "tx-lol",
	[SL_LINK_DOWN_CAUSE_RX_LOL_BIT]                     = "rx-lol",
	[SL_LINK_DOWN_CAUSE_TX_LOS_BIT]                     = "tx-los",
	[SL_LINK_DOWN_CAUSE_RX_LOS_BIT]                     = "rx-los",
	[SL_LINK_DOWN_CAUSE_FEC_CONFIG_BIT]                 = "fec-config",
	[SL_LINK_DOWN_CAUSE_LOOPBACK_IO_BIT]                = "loopback-io",
	[SL_LINK_DOWN_CAUSE_LOOPBACK_UNSUPPORTED_BIT]       = "loopback-unsupported",
	[SL_LINK_DOWN_CAUSE_PARTNER_LOOPBACK_ON_BIT]        = "partner-loopback-on",
	[SL_LINK_DOWN_CAUSE_LOOPBACK_CONFIG_BIT]            = "loopback-config",
	[SL_LINK_DOWN_CAUSE_PARTNER_LOOPBACK_MISMATCH_BIT]  = "partner-loopback-mismatch",
	[SL_LINK_DOWN_CAUSE_LOOPBACK_MEDIA_ON_BIT]          = "loopback-media-on",
	[SL_LINK_DOWN_CAUSE_MEDIA_CABLE_NOT_READY_BIT]      = "media-cable-not-ready",
	[SL_LINK_DOWN_CAUSE_MEDIA_CABLE_FAULT_BIT]          = "media-cable-fault",
	[SL_LINK_DOWN_CAUSE_MEDIA_CABLE_TRANSITION_BIT]     = "media-cable-transition",
	[SL_LINK_DOWN_CAUSE_MEDIA_CABLE_IO_BIT]             = "media-cable-io",
	[SL_LINK_DOWN_CAUSE_MEDIA_CABLE_INVALID_STATE_BIT]  = "media-cable-invalid-state",

	[SL_LINK_DOWN_RETRYABLE_BIT]                        = "retryable",
	[SL_LINK_DOWN_ORIGIN_ASYNC_BIT]                     = "origin-async",
	[SL_LINK_DOWN_ORIGIN_LINK_UP_BIT]                   = "origin-up",
};

static const struct sl_str_map sl_link_down_cause_str_map = {
	.strings     = sl_link_down_cause_strings,
	.num_strings = ARRAY_SIZE(sl_link_down_cause_strings),
};

static int sl_link_down_cause_map_masked_str(unsigned long cause_map, char *cause_str, unsigned int cause_str_size,
					     u64 mask)
{
	return sl_str_map_create(cause_map &= mask, &sl_link_down_cause_str_map,
				 cause_str, cause_str_size);
}

int sl_link_down_cause_map_str(u64 cause_map, char *cause_str, unsigned int cause_str_size)
{
	return sl_link_down_cause_map_masked_str(cause_map, cause_str, cause_str_size, ~SL_LINK_DOWN_INFO_MASK);
}
EXPORT_SYMBOL(sl_link_down_cause_map_str);

int sl_link_down_cause_map_with_info_str(u64 cause_map, char *cause_str, unsigned int cause_str_size)
{
	return sl_link_down_cause_map_masked_str(cause_map, cause_str, cause_str_size, ~0ULL);
}
EXPORT_SYMBOL(sl_link_down_cause_map_with_info_str);

int sl_link_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size)
{
	return sl_core_info_map_str(info_map, info_map_str, info_map_str_size);
}
EXPORT_SYMBOL(sl_link_info_map_str);

int sl_link_is_pml_rec_running(struct sl_link *link, bool *is_pml_rec_running)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "pml rec running get fail");
		return rtn;
	}

	return sl_ctrl_link_is_pml_rec_running(link->ldev_num, link->lgrp_num, link->num, is_pml_rec_running);
}
EXPORT_SYMBOL(sl_link_is_pml_rec_running);

const char *sl_link_config_pause_str(u32 config)
{
	if (config == SL_LINK_CONFIG_PAUSE_ASYM)
		return "pause-asym";
	if (config == SL_LINK_CONFIG_PAUSE_SYM)
		return "pause-sym";

	return "unknown";
}
EXPORT_SYMBOL(sl_link_config_pause_str);

const char *sl_link_config_hpe_str(u32 config)
{
	if (config == SL_LINK_CONFIG_HPE_LINKTRAIN)
		return "link-train";
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

	return "unknown";
}
EXPORT_SYMBOL(sl_link_config_hpe_str);
