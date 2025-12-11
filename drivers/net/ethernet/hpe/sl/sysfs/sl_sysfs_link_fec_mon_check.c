// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "data/sl_ctrl_data_link.h"

#include "sl_sysfs_link_fec_mon_check.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ucw_down_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  ucw_down_limit;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.mon_check_kobj);

	rtn = sl_ctrl_data_link_fec_mon_ucw_down_limit_get(ctrl_link, &ucw_down_limit);
	if (rtn == -EBADRQC) {
		sl_log_warn_trace(ctrl_link, LOG_BLOCK, LOG_NAME,
				  "ucw down limit show monitoring not enabled");
		return scnprintf(buf, PAGE_SIZE, "not-monitoring\n");
	}
	if (rtn) {
		sl_log_err_trace(ctrl_link, LOG_BLOCK, LOG_NAME, "fec_mon_ucw_down_limit_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "error\n");
	}

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "ucw down limit show (ucw_down_limit = %d)", ucw_down_limit);

	if (ucw_down_limit == -1)
		return scnprintf(buf, PAGE_SIZE, "calculated\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_down_limit);
}

static ssize_t ucw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  ucw_warn_limit;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.mon_check_kobj);

	rtn = sl_ctrl_data_link_fec_mon_ucw_warn_limit_get(ctrl_link, &ucw_warn_limit);
	if (rtn == -EBADRQC) {
		sl_log_warn_trace(ctrl_link, LOG_BLOCK, LOG_NAME,
				  "ucw warn limit show monitoring not enabled");
		return scnprintf(buf, PAGE_SIZE, "not-monitoring\n");
	}
	if (rtn) {
		sl_log_err_trace(ctrl_link, LOG_BLOCK, LOG_NAME, "fec_mon_ucw_warn_limit_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "error\n");
	}

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "ucw warn limit show (ucw_warn_limit = %d)", ucw_warn_limit);

	if (ucw_warn_limit == -1)
		return scnprintf(buf, PAGE_SIZE, "calculated\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ucw_warn_limit);
}

static ssize_t ccw_down_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	s32                  ccw_down_limit;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.mon_check_kobj);

	rtn = sl_ctrl_data_link_fec_mon_ccw_down_limit_get(ctrl_link, &ccw_down_limit);
	if (rtn == -EBADRQC) {
		sl_log_warn_trace(ctrl_link, LOG_BLOCK, LOG_NAME,
				  "ccw down limit show monitoring not enabled");
		return scnprintf(buf, PAGE_SIZE, "not-monitoring\n");
	}
	if (rtn) {
		sl_log_err_trace(ctrl_link, LOG_BLOCK, LOG_NAME, "fec_mon_ccw_down_limit_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "error\n");
	}

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec_mon_ccw_down_limit show (ccw_down_limit = %d)", ccw_down_limit);

	if (ccw_down_limit == -1)
		return scnprintf(buf, PAGE_SIZE, "calculated\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_down_limit);
}

static ssize_t ccw_warn_limit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  ccw_warn_limit;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.mon_check_kobj);

	rtn = sl_ctrl_data_link_fec_mon_ccw_warn_limit_get(ctrl_link, &ccw_warn_limit);
	if (rtn == -EBADRQC) {
		sl_log_warn_trace(ctrl_link, LOG_BLOCK, LOG_NAME,
				  "ccw warn limit show monitoring not enabled");
		return scnprintf(buf, PAGE_SIZE, "not-monitoring\n");
	}
	if (rtn) {
		sl_log_err_trace(ctrl_link, LOG_BLOCK, LOG_NAME, "fec_mon_ccw_warn_limit_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "error\n");
	}

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec_mon_ccw_warn_limit show (ccw_warn_limit = %d)", ccw_warn_limit);

	if (ccw_warn_limit == -1)
		return scnprintf(buf, PAGE_SIZE, "calculated\n");

	return scnprintf(buf, PAGE_SIZE, "%d\n", ccw_warn_limit);
}

static ssize_t period_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	s32                  period_ms;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.mon_check_kobj);

	rtn = sl_ctrl_data_link_fec_mon_period_ms_get(ctrl_link, &period_ms);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec_mon_period_ms_show show (period_ms = %u)", period_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", period_ms);
}

static struct kobj_attribute ucw_down_limit = __ATTR_RO(ucw_down_limit);
static struct kobj_attribute ucw_warn_limit = __ATTR_RO(ucw_warn_limit);
static struct kobj_attribute ccw_down_limit = __ATTR_RO(ccw_down_limit);
static struct kobj_attribute ccw_warn_limit = __ATTR_RO(ccw_warn_limit);
static struct kobj_attribute period_ms      = __ATTR_RO(period_ms);

static struct attribute *link_fec_mon_check_attrs[] = {
	&ucw_down_limit.attr,
	&ucw_warn_limit.attr,
	&ccw_down_limit.attr,
	&ccw_warn_limit.attr,
	&period_ms.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_mon_check);

static struct kobj_type link_fec_mon_check = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_mon_check_groups,
};

int sl_sysfs_link_fec_mon_check_create(struct sl_ctrl_link *ctrl_link)
{
	int rtn;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link fec mon check create (num = %u)", ctrl_link->num);

	rtn = kobject_init_and_add(&ctrl_link->fec.mon_check_kobj, &link_fec_mon_check,
		&ctrl_link->fec.kobj, "monitor_check");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link monitor check create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->fec.mon_check_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_fec_mon_check_delete(struct sl_ctrl_link *ctrl_link)
{
	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link fec mon check delete (num = %u)", ctrl_link->num);

	kobject_put(&ctrl_link->fec.mon_check_kobj);
}
