// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_ctl_link_priv.h"

#include "sl_sysfs_link_fec_mon_check.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ucw_down_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ucw_down_limit;
	u32                   period_ms;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_check_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ucw_down_limit = ctl_link->fec_data.info.monitor.ucw_down_limit;
	period_ms = ctl_link->fec_data.info.monitor.period_ms;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ucw_down_limit show (ucw_down_limit = %d, period = %ums)",
		ucw_down_limit, period_ms);

	if (!period_ms)
		return scnprintf(buf, PAGE_SIZE, "monitor not running\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_down_limit);
}

static ssize_t ucw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ucw_warn_limit;
	u32                   period_ms;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_check_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ucw_warn_limit = ctl_link->fec_data.info.monitor.ucw_warn_limit;
	period_ms = ctl_link->fec_data.info.monitor.period_ms;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ucw_warn_limit show (ucw_warn_limit = %d, period = %ums)",
		ucw_warn_limit, period_ms);

	if (!period_ms)
		return scnprintf(buf, PAGE_SIZE, "monitor not running\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_warn_limit);
}

static ssize_t ccw_crit_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ccw_crit_limit;
	u32                   period_ms;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_check_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ccw_crit_limit = ctl_link->fec_data.info.monitor.ccw_crit_limit;
	period_ms = ctl_link->fec_data.info.monitor.period_ms;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ccw_crit_limit show (ccw_crit_limit = %d, period = %ums)",
		ccw_crit_limit, period_ms);

	if (!period_ms)
		return scnprintf(buf, PAGE_SIZE, "monitor not running\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_crit_limit);
}

static ssize_t ccw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	s32                   ccw_warn_limit;
	u32                   period_ms;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_check_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	ccw_warn_limit = ctl_link->fec_data.info.monitor.ccw_warn_limit;
	period_ms = ctl_link->fec_data.info.monitor.period_ms;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_ccw_warn_limit show (ccw_warn_limit = %d, period = %ums)",
		ccw_warn_limit, period_ms);

	if (!period_ms)
		return scnprintf(buf, PAGE_SIZE, "monitor not running\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_warn_limit);
}

static ssize_t period_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	u32                   period_ms;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.mon_check_kobj);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	period_ms = ctl_link->fec_data.info.monitor.period_ms;
	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_mon_period_ms_show show (period_ms = %u)", period_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", period_ms);
}

static struct kobj_attribute ucw_down_limit = __ATTR_RO(ucw_down_limit);
static struct kobj_attribute ucw_warn_limit = __ATTR_RO(ucw_warn_limit);
static struct kobj_attribute ccw_crit_limit = __ATTR_RO(ccw_crit_limit);
static struct kobj_attribute ccw_warn_limit = __ATTR_RO(ccw_warn_limit);
static struct kobj_attribute period_ms      = __ATTR_RO(period_ms);

static struct attribute *link_fec_mon_check_attrs[] = {
	&ucw_down_limit.attr,
	&ucw_warn_limit.attr,
	&ccw_crit_limit.attr,
	&ccw_warn_limit.attr,
	&period_ms.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_mon_check);

static struct kobj_type link_fec_mon_check = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_mon_check_groups,
};

int sl_sysfs_link_fec_mon_check_create(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link fec mon check create (num = %u)", ctl_link->num);

	rtn = kobject_init_and_add(&ctl_link->fec.mon_check_kobj, &link_fec_mon_check,
		&ctl_link->fec.kobj, "monitor_check");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link monitor check create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_link->fec.mon_check_kobj);
		return rtn;
	}

	return 0;
}
