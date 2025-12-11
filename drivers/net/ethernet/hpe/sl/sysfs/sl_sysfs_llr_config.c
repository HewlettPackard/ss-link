// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/hpe/sl/sl_lgrp.h>

#include "sl_log.h"
#include "data/sl_ctrl_data_llr.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_ctrl_llr *ctrl_llr;
	u32                 mode;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, config_kobj);

	rtn = sl_ctrl_data_llr_config_mode_get(ctrl_llr, &mode);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME,
		   "mode (mode = %u %s)",
		   mode, sl_lgrp_llr_mode_str(mode));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_lgrp_llr_mode_str(mode));
}

static ssize_t setup_timeout_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_ctrl_llr *ctrl_llr;
	u32                 setup_timeout_ms;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, config_kobj);

	rtn = sl_ctrl_data_llr_config_setup_timeout_ms_get(ctrl_llr, &setup_timeout_ms);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME,
		   "setup timeout show (setup_timeout = %ums)",
		   setup_timeout_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", setup_timeout_ms);
}

static ssize_t start_timeout_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_ctrl_llr *ctrl_llr;
	u32                 start_timeout_ms;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, config_kobj);

	rtn = sl_ctrl_data_llr_config_start_timeout_ms_get(ctrl_llr, &start_timeout_ms);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME,
		   "start timeout show (start_timeout = %ums)",
		   start_timeout_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", start_timeout_ms);
}

static ssize_t link_down_behavior_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_ctrl_llr *ctrl_llr;
	u32                 link_dn_behavior;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, config_kobj);

	rtn = sl_ctrl_data_llr_config_link_dn_behavior_get(ctrl_llr, &link_dn_behavior);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME,
		   "down behavior show (dn_behavior = %u %s)",
		   link_dn_behavior, sl_llr_link_dn_behavior_str(link_dn_behavior));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_llr_link_dn_behavior_str(link_dn_behavior));
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

int sl_sysfs_llr_config_create(struct sl_ctrl_llr *ctrl_llr, struct kobject *parent_kobj)
{
	int rtn;

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr config create (num = %u)", ctrl_llr->num);

	rtn = kobject_init_and_add(&ctrl_llr->config_kobj, &llr_config, parent_kobj, "config");
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME,
			"llr config create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_llr->config_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_llr_config_delete(struct sl_ctrl_llr *ctrl_llr)
{
	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr config delete (num = %u)", ctrl_llr->num);

	kobject_put(&ctrl_llr->config_kobj);
}
