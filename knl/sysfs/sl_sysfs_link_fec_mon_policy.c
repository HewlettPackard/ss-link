// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_link_priv.h"
#include "sl_test_common.h"

#include "sl_sysfs_link_fec_mon_policy.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ucw_down_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ucw_down_limit;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_policy_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ucw_down_limit = ctl_link->fec_data.info.monitor.ucw_down_limit;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ucw_down_limit show (ucw_down_limit = %d)", ucw_down_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_down_limit);
}

static ssize_t ucw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ucw_warn_limit;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_policy_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ucw_warn_limit = ctl_link->fec_data.info.monitor.ucw_warn_limit;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ucw_warn_limit show (ucw_warn_limit = %d)", ucw_warn_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_warn_limit);
}

static ssize_t ccw_down_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ccw_down_limit;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_policy_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ccw_down_limit = ctl_link->fec_data.info.monitor.ccw_down_limit;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ccw_down_limit show (ccw_down_limit = %d)", ccw_down_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_down_limit);
}

static ssize_t ccw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ccw_warn_limit;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_policy_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ccw_warn_limit = ctl_link->fec_data.info.monitor.ccw_warn_limit;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ccw_warn_limit show (ccw_warn_limit = %d)", ccw_warn_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_warn_limit);
}

static struct kobj_attribute ucw_down_limit = __ATTR_RO(ucw_down_limit);
static struct kobj_attribute ucw_warn_limit = __ATTR_RO(ucw_warn_limit);
static struct kobj_attribute ccw_down_limit = __ATTR_RO(ccw_down_limit);
static struct kobj_attribute ccw_warn_limit = __ATTR_RO(ccw_warn_limit);

static struct attribute *link_fec_mon_policy_attrs[] = {
	&ucw_down_limit.attr,
	&ucw_warn_limit.attr,
	&ccw_down_limit.attr,
	&ccw_warn_limit.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_mon_policy);

static struct kobj_type link_fec_mon_policy = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_mon_policy_groups,
};

int sl_sysfs_link_fec_mon_policy_create(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link fec mon policy create (num = %u)", ctl_link->num);

	rtn = kobject_init_and_add(&ctl_link->fec.mon_policy_kobj, &link_fec_mon_policy,
		&ctl_link->fec.kobj, "monitor_policies");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link monitor policy create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_link->fec.mon_policy_kobj);
		return rtn;
	}

	return 0;
}
