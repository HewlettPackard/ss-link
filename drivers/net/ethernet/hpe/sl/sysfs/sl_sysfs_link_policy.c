// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "data/sl_ctrl_data_link.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_link_priv.h"
// FIXME: remove this header when LOCK is removed
#include "sl_test_common.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t fec_mon_period_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  fec_mon_period_ms;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	rtn = sl_ctrl_data_link_policy_fec_mon_period_ms_get(ctrl_link, &fec_mon_period_ms);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "fec_mon_period_ms show (fec_mon_period_ms = %d)", fec_mon_period_ms);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fec_mon_period_ms);
}

static ssize_t fec_mon_ucw_down_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  fec_mon_ucw_down_limit;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	rtn = sl_ctrl_data_link_policy_fec_mon_ucw_down_limit_get(ctrl_link, &fec_mon_ucw_down_limit);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "fec_mon_ucw_down_limit show (fec_mon_ucw_down_limit = %d)", fec_mon_ucw_down_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fec_mon_ucw_down_limit);
}

static ssize_t fec_mon_ucw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  fec_mon_ucw_warn_limit;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	rtn = sl_ctrl_data_link_policy_fec_mon_ucw_warn_limit_get(ctrl_link, &fec_mon_ucw_warn_limit);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "fec_mon_ucw_warn_limit show (fec_mon_ucw_warn_limit = %d)", fec_mon_ucw_warn_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fec_mon_ucw_warn_limit);
}

static ssize_t fec_mon_ccw_down_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  fec_mon_ccw_down_limit;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	rtn = sl_ctrl_data_link_policy_fec_mon_ccw_down_limit_get(ctrl_link, &fec_mon_ccw_down_limit);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "fec_mon_ccw_down_limit show (fec_mon_ccw_down_limit = %d)", fec_mon_ccw_down_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fec_mon_ccw_down_limit);
}

static ssize_t fec_mon_ccw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  fec_mon_ccw_warn_limit;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	rtn = sl_ctrl_data_link_policy_fec_mon_ccw_warn_limit_get(ctrl_link, &fec_mon_ccw_warn_limit);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "fec_mon_ccw_warn_limit show (fec_mon_ccw_warn_limit = %d)", fec_mon_ccw_warn_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fec_mon_ccw_warn_limit);
}

static ssize_t lock_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link   *ctrl_link;
	struct sl_link_policy  policy;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	sl_ctrl_link_policy_get(ctrl_link, &policy);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "link_policy_option_locked show (options = 0x%X)", policy.options);

	return scnprintf(buf, PAGE_SIZE, "%s\n", (policy.options & SL_LINK_POLICY_OPT_LOCK) ? "enabled" : "disabled");
}

static ssize_t keep_serdes_up_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link   *ctrl_link;
	struct sl_link_policy  policy;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	sl_ctrl_link_policy_get(ctrl_link, &policy);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "link_policy_option_keep_serdes_up show (options = 0x%X)", policy.options);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		(policy.options & SL_LINK_POLICY_OPT_KEEP_SERDES_UP) ? "enabled" : "disabled");
}

static ssize_t use_unsupported_cable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link   *ctrl_link;
	struct sl_link_policy  policy;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	sl_ctrl_link_policy_get(ctrl_link, &policy);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "link_policy_option_use_unsupported_cable show (options = 0x%X)", policy.options);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		(policy.options & SL_LINK_POLICY_OPT_USE_UNSUPPORTED_CABLE) ? "enabled" : "disabled");
}

static ssize_t use_supported_ss200_cable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link   *ctrl_link;
	struct sl_link_policy  policy;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	sl_ctrl_link_policy_get(ctrl_link, &policy);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		  "link_policy_option_use_supported_ss200_cable show (options = 0x%X)", policy.options);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		(policy.options & SL_LINK_POLICY_OPT_USE_SUPPORTED_SS200_CABLE) ? "enabled" : "disabled");
}

static ssize_t ignore_media_error_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link   *ctrl_link;
	struct sl_link_policy  policy;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, policy_kobj);

	sl_ctrl_link_policy_get(ctrl_link, &policy);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
	    "link_policy_option_ignore_media_error show (options = %u)", policy.options);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		(policy.options & SL_LINK_POLICY_OPT_IGNORE_MEDIA_ERROR) ? "enabled" : "disabled");
}

static struct kobj_attribute fec_mon_period_ms         = __ATTR_RO(fec_mon_period_ms);
static struct kobj_attribute fec_mon_ucw_down_limit    = __ATTR_RO(fec_mon_ucw_down_limit);
static struct kobj_attribute fec_mon_ucw_warn_limit    = __ATTR_RO(fec_mon_ucw_warn_limit);
static struct kobj_attribute fec_mon_ccw_down_limit    = __ATTR_RO(fec_mon_ccw_down_limit);
static struct kobj_attribute fec_mon_ccw_warn_limit    = __ATTR_RO(fec_mon_ccw_warn_limit);
static struct kobj_attribute lock                      = __ATTR_RO(lock);
static struct kobj_attribute keep_serdes_up            = __ATTR_RO(keep_serdes_up);
static struct kobj_attribute use_unsupported_cable     = __ATTR_RO(use_unsupported_cable);
static struct kobj_attribute use_supported_ss200_cable = __ATTR_RO(use_supported_ss200_cable);
static struct kobj_attribute ignore_media_error        = __ATTR_RO(ignore_media_error);

static struct attribute *link_policy_attrs[] = {
	&fec_mon_period_ms.attr,
	&fec_mon_ucw_down_limit.attr,
	&fec_mon_ucw_warn_limit.attr,
	&fec_mon_ccw_down_limit.attr,
	&fec_mon_ccw_warn_limit.attr,
	&lock.attr,
	&keep_serdes_up.attr,
	&use_unsupported_cable.attr,
	&use_supported_ss200_cable.attr,
	&ignore_media_error.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_policy);

static struct kobj_type link_policy = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_policy_groups,
};

int sl_sysfs_link_policy_create(struct sl_ctrl_link *ctrl_link)
{
	int rtn;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link policy create (num = %u)", ctrl_link->num);

	rtn = kobject_init_and_add(&ctrl_link->policy_kobj, &link_policy, &ctrl_link->kobj, "policies");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link policy create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->policy_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_policy_delete(struct sl_ctrl_link *ctrl_link)
{
	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link config delete (num = %u)", ctrl_link->num);

	kobject_put(&ctrl_link->policy_kobj);
}
