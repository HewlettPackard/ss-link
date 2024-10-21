// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include <linux/sl_lgrp.h>
#include "sl_ctl_llr.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, config_kobj);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"mode (llr = 0x%p, mode = %u %s)",
		ctl_llr, ctl_llr->config.mode, sl_lgrp_llr_mode_str(ctl_llr->config.mode));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_lgrp_llr_mode_str(ctl_llr->config.mode));
}

static ssize_t setup_timeout_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, config_kobj);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"setup timeout show (llr = 0x%p, setup_timeout = %ums)",
		ctl_llr, ctl_llr->config.setup_timeout_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", ctl_llr->config.setup_timeout_ms);
}

static ssize_t start_timeout_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, config_kobj);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"start timeout show (llr = 0x%p, start_timeout = %ums)",
		ctl_llr, ctl_llr->config.start_timeout_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", ctl_llr->config.start_timeout_ms);
}

static ssize_t link_down_behavior_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, config_kobj);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"down behavior show (llr = 0x%p, dn_behavior = %u %s)",
		ctl_llr, ctl_llr->config.link_dn_behavior,
		sl_llr_link_dn_behavior_str(ctl_llr->config.link_dn_behavior));

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		sl_llr_link_dn_behavior_str(ctl_llr->config.link_dn_behavior));
}

// FIXME: add options here when/if any are defined

static struct kobj_attribute mode             = __ATTR_RO(mode);
static struct kobj_attribute setup_timeout_ms = __ATTR_RO(setup_timeout_ms);
static struct kobj_attribute start_timeout_ms = __ATTR_RO(start_timeout_ms);
static struct kobj_attribute dn_behavior      = __ATTR_RO(link_down_behavior);

static struct attribute *llr_config_attrs[] = {
	&mode.attr,
	&setup_timeout_ms.attr,
	&start_timeout_ms.attr,
	&dn_behavior.attr,
	NULL
};
ATTRIBUTE_GROUPS(llr_config);

static struct kobj_type llr_config = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = llr_config_groups,
};

int sl_sysfs_llr_config_create(struct sl_ctl_llr *ctl_llr)
{
	int rtn;

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "llr config create (num = %u)", ctl_llr->num);

	rtn = kobject_init_and_add(&ctl_llr->config_kobj, &llr_config, &ctl_llr->kobj, "config");
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr config create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_llr->config_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_llr_config_delete(struct sl_ctl_llr *ctl_llr)
{
	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "llr config delete (num = %u)", ctl_llr->num);

	kobject_put(&ctl_llr->config_kobj);
}
