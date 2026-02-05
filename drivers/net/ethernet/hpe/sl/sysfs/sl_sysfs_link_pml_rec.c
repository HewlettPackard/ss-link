// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/types.h>

#include "sl_log.h"
#include "data/sl_core_data_link.h"

#include "sl_sysfs.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t pml_rec_attempts_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  attempts;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	sl_core_data_link_pml_rec_attempts_get(core_link, &attempts);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec attempts show (attempts = %d)", attempts);

	return scnprintf(buf, PAGE_SIZE, "%d\n", attempts);
}

static ssize_t pml_rec_successes_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  successes;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	sl_core_data_link_pml_rec_successes_get(core_link, &successes);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec successes show (successes = %d)", successes);

	return scnprintf(buf, PAGE_SIZE, "%d\n", successes);
}

static ssize_t pml_rec_link_fault_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_fault_cause;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	sl_core_data_link_pml_rec_link_fault_cause_get(core_link, &link_fault_cause);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_fault_cause show (link_fault_cause = %d)", link_fault_cause);

	return scnprintf(buf, PAGE_SIZE, "%d\n", link_fault_cause);
}

static ssize_t pml_rec_link_down_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_down_cause;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	sl_core_data_link_pml_rec_link_down_cause_get(core_link, &link_down_cause);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_down_cause show (link_down_cause = %d)", link_down_cause);

	return scnprintf(buf, PAGE_SIZE, "%d\n", link_down_cause);
}

static ssize_t pml_rec_link_fault_failed_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_fault_failed_cause;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	sl_core_data_link_pml_rec_link_fault_failed_cause_get(core_link, &link_fault_failed_cause);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_fault_failed_cause show (link_fault_failed_cause = %d)",
			link_fault_failed_cause);

	return scnprintf(buf, PAGE_SIZE, "%d\n", link_fault_failed_cause);
}

static ssize_t pml_rec_link_down_failed_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_down_failed_cause;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	sl_core_data_link_pml_rec_link_down_failed_cause_get(core_link, &link_down_failed_cause);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_down_cause show (link_down_failed_cause = %d)",
			link_down_failed_cause);

	return scnprintf(buf, PAGE_SIZE, "%d\n", link_down_failed_cause);
}

static ssize_t pml_rec_rate_limit_exceeded_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  rate_limit_exceeded;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	sl_core_data_link_pml_rec_rate_limit_exceeded_get(core_link, &rate_limit_exceeded);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec rate_limit_exceeded show (rate_limit_exceeded = %d)",
			rate_limit_exceeded);

	return scnprintf(buf, PAGE_SIZE, "%d\n", rate_limit_exceeded);
}

static struct kobj_attribute link_pml_rec_attempts                = __ATTR_RO(pml_rec_attempts);
static struct kobj_attribute link_pml_rec_successes               = __ATTR_RO(pml_rec_successes);
static struct kobj_attribute link_pml_rec_link_fault_cause        = __ATTR_RO(pml_rec_link_fault_cause);
static struct kobj_attribute link_pml_rec_link_down_cause         = __ATTR_RO(pml_rec_link_down_cause);
static struct kobj_attribute link_pml_rec_link_fault_failed_cause = __ATTR_RO(pml_rec_link_fault_failed_cause);
static struct kobj_attribute link_pml_rec_link_down_failed_cause  = __ATTR_RO(pml_rec_link_down_failed_cause);
static struct kobj_attribute link_pml_rec_rate_limit_exceeded     = __ATTR_RO(pml_rec_rate_limit_exceeded);

static struct attribute *link_pml_rec_attrs[] = {
	&link_pml_rec_attempts.attr,
	&link_pml_rec_successes.attr,
	&link_pml_rec_link_fault_cause.attr,
	&link_pml_rec_link_down_cause.attr,
	&link_pml_rec_link_fault_failed_cause.attr,
	&link_pml_rec_link_down_failed_cause.attr,
	&link_pml_rec_rate_limit_exceeded.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_pml_rec);

static struct kobj_type link_pml_rec = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_pml_rec_groups,
};

int sl_sysfs_link_pml_rec_create(struct sl_core_link *core_link, struct kobject *parent_kobj)
{
	int rtn;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "link pml rec create (num = %u)", core_link->num);

	rtn = kobject_init_and_add(&core_link->pml_rec_kobj, &link_pml_rec, parent_kobj, "pml_recovery");
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME,
			   "link pml rec create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&core_link->pml_rec_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_pml_rec_delete(struct sl_core_link *core_link)
{
	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "link pml rec delete (num = %u)", core_link->num);

	kobject_put(&core_link->pml_rec_kobj);
}
