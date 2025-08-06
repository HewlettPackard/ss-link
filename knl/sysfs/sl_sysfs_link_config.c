// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_core_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_link_priv.h"
#include "sl_test_common.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t link_up_timeout_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_link_config config;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_ctl_link_config_get(ctl_link, &config);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
	    "link up timeout show (link_up_timeout = %ums)", config.link_up_timeout_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", config.link_up_timeout_ms);
}

static ssize_t link_up_tries_max_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_link_config config;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_ctl_link_config_get(ctl_link, &config);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
	    "link up tries max show (link_up_tries_max = %d)", config.link_up_tries_max);

	if (config.link_up_tries_max == SL_LINK_INFINITE_UP_TRIES)
		return scnprintf(buf, PAGE_SIZE, "infinite\n");
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", config.link_up_tries_max);
}

static ssize_t fec_up_settle_wait_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_link_config config;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_ctl_link_config_get(ctl_link, &config);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
	    "fec_up_settle_wait show (fec_up_settle_wait = %dms)", config.fec_up_settle_wait_ms);

	return scnprintf(buf, PAGE_SIZE, "%d\n", config.fec_up_settle_wait_ms);
}

static ssize_t fec_up_check_wait_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_link_config config;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_ctl_link_config_get(ctl_link, &config);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
	    "fec_up_check_wait show (fec_up_check_wait = %ums)", config.fec_up_check_wait_ms);

	return scnprintf(buf, PAGE_SIZE, "%d\n", config.fec_up_check_wait_ms);
}

static ssize_t fec_up_ucw_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_link_config config;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_ctl_link_config_get(ctl_link, &config);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
	    "fec_up_ucw_limit show (fec_up_ucw_limit = %d)", config.fec_up_ucw_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", config.fec_up_ucw_limit);
}

static ssize_t fec_up_ccw_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_link_config config;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_ctl_link_config_get(ctl_link, &config);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
	    "fec_up_ccw_limit show (fec_up_ccw_limit = %d)", config.fec_up_ccw_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", config.fec_up_ccw_limit);
}

static ssize_t lock_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_link_config config;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_ctl_link_config_get(ctl_link, &config);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
	    "link_config_option_locked show (options = %u)", config.options);

	return scnprintf(buf, PAGE_SIZE, "%s\n", (config.options & SL_LINK_CONFIG_OPT_LOCK) ? "enabled" : "disabled");
}

static ssize_t pause_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	int                 idx;
	char                output[20];
	u32                 pause_map;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	pause_map = ctl_link->config.pause_map;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"pause map show (link = 0x%p, map = 0x%X)", ctl_link, pause_map);

	if (pause_map == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	idx = 0;
	if (is_flag_set(pause_map, SL_LINK_CONFIG_PAUSE_ASYM))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_pause_str(SL_LINK_CONFIG_PAUSE_ASYM));
	if (is_flag_set(pause_map, SL_LINK_CONFIG_PAUSE_SYM))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_pause_str(SL_LINK_CONFIG_PAUSE_SYM));
	output[idx - 1] = '\0';

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t hpe_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	int                 idx;
	char                output[80];
	u32                 hpe_map;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	hpe_map = ctl_link->config.hpe_map;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"hpe map show (link = 0x%p, map = 0x%X)", ctl_link, hpe_map);

	if (hpe_map == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	idx = 0;
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_LINKTRAIN));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_PRECODING))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_PRECODING));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_PCAL))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_PCAL));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_R3))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R3));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_R2))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R2));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_C3))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R3));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_C2))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R2));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_LLR))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_LLR));
	output[idx - 1] = '\0';

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t autoneg_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"autoneg show (link = 0x%p, options = 0x%X)", ctl_link, ctl_link->config.options);

	if (is_flag_set(ctl_link->config.options, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE))
		return scnprintf(buf, PAGE_SIZE, "enabled\n");
	if (is_flag_set(ctl_link->config.options, SL_LINK_CONFIG_OPT_AUTONEG_CONTINUOUS_ENABLE))
		return scnprintf(buf, PAGE_SIZE, "enabled-continuous\n");

	return scnprintf(buf, PAGE_SIZE, "disabled\n");
}

static ssize_t loopback_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"loopback show (link = 0x%p, options = 0x%X)", ctl_link, ctl_link->config.options);

	if (is_flag_set(ctl_link->config.options, SL_LINK_CONFIG_OPT_HEADSHELL_LOOPBACK_ENABLE))
		return scnprintf(buf, PAGE_SIZE, "enabled-headshell\n");
	if (is_flag_set(ctl_link->config.options, SL_LINK_CONFIG_OPT_REMOTE_LOOPBACK_ENABLE))
		return scnprintf(buf, PAGE_SIZE, "enabled-remote\n");

	return scnprintf(buf, PAGE_SIZE, "disabled\n");
}

static ssize_t extended_reach_force_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = container_of(kobj, struct sl_ctl_link, config_kobj);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"extended reach force show (link = 0x%p, options = 0x%X)", ctl_link, ctl_link->config.options);

	if (is_flag_set(ctl_link->config.options, SL_LINK_CONFIG_OPT_EXTENDED_REACH_FORCE))
		return scnprintf(buf, PAGE_SIZE, "enabled\n");

	return scnprintf(buf, PAGE_SIZE, "disabled\n");
}

static ssize_t auto_lane_degrade_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"auto lane degrade show (link = 0x%p, options = 0x%X)", ctl_link, ctl_link->config.options);

	if (is_flag_set(ctl_link->config.options, SL_LINK_CONFIG_OPT_ALD_ENABLE))
		return scnprintf(buf, PAGE_SIZE, "enabled\n");

	return scnprintf(buf, PAGE_SIZE, "disabled\n");
}

static struct kobj_attribute link_up_timeout_ms    = __ATTR_RO(link_up_timeout_ms);
static struct kobj_attribute link_up_tries_max_ms  = __ATTR_RO(link_up_tries_max);
static struct kobj_attribute fec_up_settle_wait_ms = __ATTR_RO(fec_up_settle_wait_ms);
static struct kobj_attribute fec_up_check_wait_ms  = __ATTR_RO(fec_up_check_wait_ms);
static struct kobj_attribute fec_up_ucw_limit      = __ATTR_RO(fec_up_ucw_limit);
static struct kobj_attribute fec_up_ccw_limit      = __ATTR_RO(fec_up_ccw_limit);
static struct kobj_attribute lock                  = __ATTR_RO(lock);
static struct kobj_attribute pause_map             = __ATTR_RO(pause_map);
static struct kobj_attribute hpe_map               = __ATTR_RO(hpe_map);
static struct kobj_attribute autoneg               = __ATTR_RO(autoneg);
static struct kobj_attribute loopback              = __ATTR_RO(loopback);
static struct kobj_attribute extended_reach_force  = __ATTR_RO(extended_reach_force);
static struct kobj_attribute auto_lane_degrade     = __ATTR_RO(auto_lane_degrade);

static struct attribute *link_config_attrs[] = {
	&link_up_timeout_ms.attr,
	&link_up_tries_max_ms.attr,
	&fec_up_settle_wait_ms.attr,
	&fec_up_check_wait_ms.attr,
	&fec_up_ucw_limit.attr,
	&fec_up_ccw_limit.attr,
	&lock.attr,
	&pause_map.attr,
	&hpe_map.attr,
	&autoneg.attr,
	&loopback.attr,
	&extended_reach_force.attr,
	&auto_lane_degrade.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_config);

static struct kobj_type link_config = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_config_groups,
};

int sl_sysfs_link_config_create(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link config create (num = %u)", ctl_link->num);

	rtn = kobject_init_and_add(&ctl_link->config_kobj, &link_config, &ctl_link->kobj, "config");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link config create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_link->config_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_config_delete(struct sl_ctl_link *ctl_link)
{
	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link config delete (num = %u)", ctl_link->num);

	kobject_put(&ctl_link->config_kobj);
}
