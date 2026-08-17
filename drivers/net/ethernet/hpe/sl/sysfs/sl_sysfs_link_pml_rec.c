// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/types.h>

#include "sl_log.h"
#include "data/sl_core_data_link.h"

#include "sl_sysfs.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

#ifdef CONFIG_SYSFS
static ssize_t pml_rec_attempts_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  attempts;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_attempts_get(core_link, &attempts);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec attempts show (attempts = %d)", attempts);

	return sysfs_emit(buf, "%d\n", attempts);
}

static ssize_t pml_rec_successes_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  successes;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_successes_get(core_link, &successes);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec successes show (successes = %d)", successes);

	return sysfs_emit(buf, "%d\n", successes);
}

static ssize_t pml_rec_link_fault_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_fault_cause;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_link_fault_cause_get(core_link, &link_fault_cause);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_fault_cause show (link_fault_cause = %d)", link_fault_cause);

	return sysfs_emit(buf, "%d\n", link_fault_cause);
}

static ssize_t pml_rec_link_down_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_down_cause;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_link_down_cause_get(core_link, &link_down_cause);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_down_cause show (link_down_cause = %d)", link_down_cause);

	return sysfs_emit(buf, "%d\n", link_down_cause);
}

static ssize_t pml_rec_link_fault_failed_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_fault_failed_cause;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_link_fault_failed_cause_get(core_link, &link_fault_failed_cause);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_fault_failed_cause show (link_fault_failed_cause = %d)",
		   link_fault_failed_cause);

	return sysfs_emit(buf, "%d\n", link_fault_failed_cause);
}

static ssize_t pml_rec_link_down_failed_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_down_failed_cause;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_link_down_failed_cause_get(core_link, &link_down_failed_cause);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_down_cause show (link_down_failed_cause = %d)",
		   link_down_failed_cause);

	return sysfs_emit(buf, "%d\n", link_down_failed_cause);
}

static ssize_t pml_rec_link_remote_fault_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_remote_fault_cause;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_link_remote_fault_cause_get(core_link, &link_remote_fault_cause);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec link_remote_fault_cause show (link_remote_fault_cause = %d)",
		   link_remote_fault_cause);

	return sysfs_emit(buf, "%d\n", link_remote_fault_cause);
}

static ssize_t pml_rec_link_remote_fault_failed_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  link_remote_fault_failed_cause;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_link_remote_fault_failed_cause_get(core_link, &link_remote_fault_failed_cause);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "pml rec link_remote_fault_failed_cause show (link_remote_fault_failed_cause = %d)",
		   link_remote_fault_failed_cause);

	return sysfs_emit(buf, "%d\n", link_remote_fault_failed_cause);
}

static ssize_t pml_rec_rate_limit_exceeded_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	int                  rate_limit_exceeded;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_rate_limit_exceeded_get(core_link, &rate_limit_exceeded);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec rate_limit_exceeded show (rate_limit_exceeded = %d)",
		   rate_limit_exceeded);

	return sysfs_emit(buf, "%d\n", rate_limit_exceeded);
}

static ssize_t pml_rec_last_down_cause_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	u64                  down_cause_map;
	time64_t             down_time;
	char                 cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_last_down_cause_map_info_get(core_link, &down_cause_map, &down_time);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_core_data_link_pml_rec_down_cause_map_str(down_cause_map, cause_str, sizeof(cause_str));

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "pml rec last down cause map show (cause_map = 0x%llX %s)",
		   down_cause_map, cause_str);

	return sysfs_emit(buf, "%s\n", cause_str);
}

static ssize_t pml_rec_last_down_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_core_link *core_link;
	u64                  down_cause_map;
	time64_t             down_time;
	int		     rtn;

	core_link = container_of(kobj, struct sl_core_link, pml_rec_kobj);

	rtn = sl_core_data_link_pml_rec_last_down_cause_map_info_get(core_link, &down_cause_map, &down_time);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	if (down_cause_map == PML_REC_DOWN_CAUSE_INVALID)
		return sysfs_emit(buf, "none\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "pml rec last down time show (time = %lld %ptTt %ptTd)", down_time, &down_time, &down_time);

	return sysfs_emit(buf, "%ptTt %ptTd\n", &down_time, &down_time);
}

static struct kobj_attribute link_pml_rec_attempts                       = __ATTR_RO(pml_rec_attempts);
static struct kobj_attribute link_pml_rec_successes                      = __ATTR_RO(pml_rec_successes);
static struct kobj_attribute link_pml_rec_link_fault_cause               = __ATTR_RO(pml_rec_link_fault_cause);
static struct kobj_attribute link_pml_rec_link_down_cause                = __ATTR_RO(pml_rec_link_down_cause);
static struct kobj_attribute link_pml_rec_link_remote_fault_cause        = __ATTR_RO(pml_rec_link_remote_fault_cause);
static struct kobj_attribute link_pml_rec_link_fault_failed_cause        = __ATTR_RO(pml_rec_link_fault_failed_cause);
static struct kobj_attribute link_pml_rec_link_down_failed_cause         = __ATTR_RO(pml_rec_link_down_failed_cause);
static struct kobj_attribute link_pml_rec_link_remote_fault_failed_cause = __ATTR_RO(pml_rec_link_remote_fault_failed_cause);
static struct kobj_attribute link_pml_rec_rate_limit_exceeded            = __ATTR_RO(pml_rec_rate_limit_exceeded);
static struct kobj_attribute link_pml_rec_last_down_cause_map            = __ATTR_RO(pml_rec_last_down_cause_map);
static struct kobj_attribute link_pml_rec_last_down_time                 = __ATTR_RO(pml_rec_last_down_time);

static struct attribute *link_pml_rec_attrs[] = {
	&link_pml_rec_attempts.attr,
	&link_pml_rec_successes.attr,
	&link_pml_rec_link_fault_cause.attr,
	&link_pml_rec_link_down_cause.attr,
	&link_pml_rec_link_remote_fault_cause.attr,
	&link_pml_rec_link_fault_failed_cause.attr,
	&link_pml_rec_link_down_failed_cause.attr,
	&link_pml_rec_link_remote_fault_failed_cause.attr,
	&link_pml_rec_rate_limit_exceeded.attr,
	&link_pml_rec_last_down_cause_map.attr,
	&link_pml_rec_last_down_time.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_pml_rec);

static struct kobj_type link_pml_rec = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_pml_rec_groups,
};
#endif /* CONFIG_SYSFS */

int sl_sysfs_link_pml_rec_create(struct sl_core_link *core_link, struct kobject *parent_kobj)
{
#ifdef CONFIG_SYSFS
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
#else /* CONFIG_SYSFS */
	return 0;
#endif /* CONFIG_SYSFS */
}

void sl_sysfs_link_pml_rec_delete(struct sl_core_link *core_link)
{
#ifdef CONFIG_SYSFS
	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "link pml rec delete (num = %u)", core_link->num);

	kobject_put(&core_link->pml_rec_kobj);
#endif /* CONFIG_SYSFS */
}
