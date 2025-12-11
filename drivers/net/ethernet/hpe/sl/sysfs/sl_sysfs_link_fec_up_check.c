// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_core_link.h"
#include "data/sl_core_data_link.h"

#include "sl_sysfs_link_fec_up_check.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ucw_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	s32                  ucw_limit;

	core_link = container_of(kobj, struct sl_core_link, fec.up_check_kobj);

	rtn = sl_core_data_link_fec_up_ucw_limit_get(core_link, &ucw_limit);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "fec_up_ucw_limit show (ucw_limit = %d)", ucw_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_limit);
}

static ssize_t ccw_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	s32                  ccw_limit;

	core_link = container_of(kobj, struct sl_core_link, fec.up_check_kobj);

	rtn = sl_core_data_link_fec_up_ccw_limit_get(core_link, &ccw_limit);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "fec_up_ccw_limit show (ccw_limit = %d)", ccw_limit);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_limit);
}

static ssize_t settle_wait_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	u32                  settle_wait_ms;

	core_link = container_of(kobj, struct sl_core_link, fec.up_check_kobj);

	rtn = sl_core_data_link_fec_up_settle_wait_ms_get(core_link, &settle_wait_ms);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "settle_wait_ms show (settle_wait_ms = %u)", settle_wait_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", settle_wait_ms);
}

static ssize_t check_wait_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	u32                  check_wait_ms;

	core_link = container_of(kobj, struct sl_core_link, fec.up_check_kobj);

	rtn = sl_core_data_link_fec_up_check_wait_ms_get(core_link, &check_wait_ms);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
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

int sl_sysfs_link_fec_up_check_create(struct sl_core_link *core_link, struct kobject *parent_kobj)
{
	int rtn;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "link fec up check create (num = %u)", core_link->num);

	rtn = kobject_init_and_add(&core_link->fec.up_check_kobj, &link_fec_up_check, parent_kobj, "up_check");
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME,
			   "link up_check create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&core_link->fec.up_check_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_fec_up_check_delete(struct sl_core_link *core_link)
{
	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "link fec up check delete (num = %u)", core_link->num);

	kobject_put(&core_link->fec.up_check_kobj);
}
