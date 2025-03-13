// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_core_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_link_counters.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t link_up_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_UP_CMD);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link up cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_retry_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_UP_RETRY);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link up retry show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_UP);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link up show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_UP_FAIL);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link up fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_DOWN_CMD);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link down cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_DOWN);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link down show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_canceled_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	u32                 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_UP_CANCELED);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link up canceled show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_cancel_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	u32                 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_UP_CANCEL_CMD);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link up cancel cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_reset_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_RESET_CMD);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link reset cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_fault_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32 counter;

	ctl_link = container_of(kobj, struct sl_ctl_link, counters_kobj);

	counter = sl_ctl_link_counters_get(ctl_link, LINK_FAULT);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link fault show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static struct kobj_attribute link_up_cmd               = __ATTR_RO(link_up_cmd);
static struct kobj_attribute link_up_retry             = __ATTR_RO(link_up_retry);
static struct kobj_attribute link_up                   = __ATTR_RO(link_up);
static struct kobj_attribute link_up_fail              = __ATTR_RO(link_up_fail);
static struct kobj_attribute link_down_cmd             = __ATTR_RO(link_down_cmd);
static struct kobj_attribute link_down                 = __ATTR_RO(link_down);
static struct kobj_attribute link_up_cancel_cmd        = __ATTR_RO(link_up_cancel_cmd);
static struct kobj_attribute link_up_canceled          = __ATTR_RO(link_up_canceled);
static struct kobj_attribute link_reset_cmd            = __ATTR_RO(link_reset_cmd);
static struct kobj_attribute link_fault                = __ATTR_RO(link_fault);

static struct attribute *link_counters_attrs[] = {
	&link_up_cmd.attr,
	&link_up_retry.attr,
	&link_up.attr,
	&link_up_fail.attr,
	&link_down_cmd.attr,
	&link_down.attr,
	&link_up_cancel_cmd.attr,
	&link_up_canceled.attr,
	&link_reset_cmd.attr,
	&link_fault.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_counters);

static struct kobj_type link_counters = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_counters_groups,
};

int sl_sysfs_link_counters_create(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link counters create");

	rtn = kobject_init_and_add(&ctl_link->counters_kobj, &link_counters, &ctl_link->kobj, "counters");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link counters create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_link->counters_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_counters_delete(struct sl_ctl_link *ctl_link)
{
	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link counters delete");
	kobject_put(&ctl_link->counters_kobj);
}
