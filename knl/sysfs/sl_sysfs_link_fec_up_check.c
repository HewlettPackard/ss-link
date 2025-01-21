// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_link_priv.h"
#include "sl_core_link.h"

#include "sl_sysfs_link_fec_up_check.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ucw_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_core_link  *core_link;
	s32                   ucw_limit;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_check_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num);

	spin_lock_irqsave(&core_link->data_lock, irq_flags);
	ucw_limit = core_link->config.fec_up_ucw_limit;
	spin_unlock_irqrestore(&core_link->data_lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_up_ucw_limit show (ucw_limit = %d)", ucw_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_limit);
}

static ssize_t ccw_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_core_link  *core_link;
	s32                   ccw_limit;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_check_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num);

	spin_lock_irqsave(&core_link->data_lock, irq_flags);
	ccw_limit = core_link->config.fec_up_ccw_limit;
	spin_unlock_irqrestore(&core_link->data_lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"fec_up_ccw_limit show (ccw_limit = %d)", ccw_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_limit);
}

static ssize_t settle_wait_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_core_link  *core_link;
	u32                   settle_wait_ms;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_check_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num);

	spin_lock_irqsave(&core_link->data_lock, irq_flags);
	settle_wait_ms = core_link->config.fec_up_settle_wait_ms;
	spin_unlock_irqrestore(&core_link->data_lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"settle_wait_ms show (settle_wait_ms = %u)", settle_wait_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", settle_wait_ms);
}

static ssize_t check_wait_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link   *ctl_link;
	struct sl_core_link  *core_link;
	u32                   check_wait_ms;
	unsigned long         irq_flags;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_check_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num);

	spin_lock_irqsave(&core_link->data_lock, irq_flags);
	check_wait_ms = core_link->config.fec_up_check_wait_ms;
	spin_unlock_irqrestore(&core_link->data_lock, irq_flags);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"check_wait_ms show (check_wait_ms = %u)", check_wait_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", check_wait_ms);
}

static struct kobj_attribute ucw_limit      = __ATTR_RO(ucw_limit);
static struct kobj_attribute ccw_limit      = __ATTR_RO(ccw_limit);
static struct kobj_attribute settle_wait_ms = __ATTR_RO(settle_wait_ms);
static struct kobj_attribute check_wait_ms  = __ATTR_RO(check_wait_ms);

static struct attribute *link_fec_up_check_attrs[] = {
	&ucw_limit.attr,
	&ccw_limit.attr,
	&settle_wait_ms.attr,
	&check_wait_ms.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_up_check);

static struct kobj_type link_fec_up_check = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_up_check_groups,
};

int sl_sysfs_link_fec_up_check_create(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link fec up check create (num = %u)", ctl_link->num);

	rtn = kobject_init_and_add(&ctl_link->fec.up_check_kobj, &link_fec_up_check,
		&ctl_link->fec.kobj, "up_check");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link up_check create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_link->fec.mon_check_kobj);
		return rtn;
	}

	return 0;
}
