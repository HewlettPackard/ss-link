// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_link_priv.h"
#include "sl_core_link.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	u32                 state;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_ctl_link_state_get_cmd(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num, &state);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"state show (link = 0x%p, state = %u %s)", ctl_link, state, sl_link_state_str(state));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_link_state_str(state));
}

static ssize_t speed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32                  state;
	u32                  speed;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_ctl_link_state_get_cmd(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num, &state);
	sl_core_link_speed_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num, &speed);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"speed show (state = %u %s, speed = 0x%X %s)",
		state, sl_link_state_str(state), speed, sl_lgrp_config_tech_str(speed));

	if (state != SL_LINK_STATE_UP)
		return scnprintf(buf, PAGE_SIZE, "not up\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_lgrp_config_tech_str(speed));
}

static ssize_t last_up_fail_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	u32                 state;
	u32                 up_fail_cause;
	time64_t            up_fail_time;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_core_link_last_up_fail_cause_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &up_fail_cause, &up_fail_time);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"last up fail cause show (cause = %u %s)", up_fail_cause, sl_link_down_cause_str(up_fail_cause));

	if (up_fail_cause == SL_LINK_DOWN_CAUSE_INVALID) {
		sl_ctl_link_state_get_cmd(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &state);
		if (state == SL_LINK_STATE_UP)
			return scnprintf(buf, PAGE_SIZE, "no fail\n");
		else
			return scnprintf(buf, PAGE_SIZE, "no record\n");
	}

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_link_down_cause_str(up_fail_cause));
}

static ssize_t last_up_fail_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	u32                 state;
	u32                 up_fail_cause;
	time64_t            up_fail_time;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_core_link_last_up_fail_cause_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &up_fail_cause, &up_fail_time);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"last up fail time show (cause = %u %s, time = %lld %ptTt %ptTd)",
		up_fail_cause, sl_link_down_cause_str(up_fail_cause), up_fail_time, &up_fail_time, &up_fail_time);

	if (up_fail_cause == SL_LINK_DOWN_CAUSE_INVALID) {
		sl_ctl_link_state_get_cmd(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &state);
		if (state == SL_LINK_STATE_UP)
			return scnprintf(buf, PAGE_SIZE, "no fail\n");
		else
			return scnprintf(buf, PAGE_SIZE, "no record\n");
	}

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &up_fail_time, &up_fail_time);
}

static ssize_t last_down_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32                  down_cause;
	time64_t             down_time;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_core_link_last_down_cause_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &down_cause, &down_time);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"last down cause show (cause = %u %s)", down_cause, sl_link_down_cause_str(down_cause));

	if (down_cause == SL_LINK_DOWN_CAUSE_INVALID)
		return scnprintf(buf, PAGE_SIZE, "no record\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_link_down_cause_str(down_cause));
}

static ssize_t last_down_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32                  down_cause;
	time64_t             down_time;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_core_link_last_down_cause_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &down_cause, &down_time);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"last down time show (cause = %u %s, time = %lld %ptTt %ptTd)",
		down_cause, sl_link_down_cause_str(down_cause), down_time, &down_time, &down_time);

	if (down_cause == SL_LINK_DOWN_CAUSE_INVALID)
		return scnprintf(buf, PAGE_SIZE, "no record\n");

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &down_time, &down_time);
}

static ssize_t ccw_warn_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	bool                value;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_core_link_ccw_warn_limit_crossed_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &value);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"ccw warn limit crossed show (value = %d %s)", value, value ? "yes":"no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", value ? "yes":"no");
}

static ssize_t ccw_crit_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link *ctl_link;
	bool                value;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_core_link_ccw_crit_limit_crossed_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &value);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"ccw crit limit crossed show (value = %d %s)", value, value ? "yes":"no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", value ? "yes":"no");
}

static ssize_t up_count_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32                  up_count;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_ctl_link_up_count_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &up_count);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "up count show (up_count = %u)", up_count);

	return scnprintf(buf, PAGE_SIZE, "%u\n", up_count);
}

static ssize_t up_time_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32                  up_time_ms;
	u32                  total_time_ms;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_ctl_link_up_clocks_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
		&up_time_ms, &total_time_ms);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "up time show (up_time_ms = %u, total_time_ms = %u)",
		up_time_ms, total_time_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", up_time_ms);
}

static ssize_t total_time_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	u32                  up_time_ms;
	u32                  total_time_ms;

	ctl_link = container_of(kobj, struct sl_ctl_link, kobj);

	sl_ctl_link_up_clocks_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
		&up_time_ms, &total_time_ms);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "total time show (up_time_ms = %u, total_time_ms = %u)",
		up_time_ms, total_time_ms);

	return scnprintf(buf, PAGE_SIZE, "%u\n", total_time_ms);
}

static struct kobj_attribute link_state                  = __ATTR_RO(state);
static struct kobj_attribute link_speed                  = __ATTR_RO(speed);
static struct kobj_attribute link_last_up_fail_cause     = __ATTR_RO(last_up_fail_cause);
static struct kobj_attribute link_last_up_fail_time      = __ATTR_RO(last_up_fail_time);
static struct kobj_attribute link_last_down_cause        = __ATTR_RO(last_down_cause);
static struct kobj_attribute link_last_down_time         = __ATTR_RO(last_down_time);
static struct kobj_attribute link_ccw_warn_limit_crossed = __ATTR_RO(ccw_warn_limit_crossed);
static struct kobj_attribute link_ccw_crit_limit_crossed = __ATTR_RO(ccw_crit_limit_crossed);
static struct kobj_attribute link_up_count               = __ATTR_RO(up_count);
static struct kobj_attribute link_up_time_ms             = __ATTR_RO(up_time_ms);
static struct kobj_attribute link_total_time_ms          = __ATTR_RO(total_time_ms);

static struct attribute *link_attrs[] = {
	&link_state.attr,
	&link_speed.attr,
	&link_last_up_fail_cause.attr,
	&link_last_up_fail_time.attr,
	&link_last_down_cause.attr,
	&link_last_down_time.attr,
	&link_ccw_warn_limit_crossed.attr,
	&link_ccw_crit_limit_crossed.attr,
	&link_up_count.attr,
	&link_up_time_ms.attr,
	&link_total_time_ms.attr,
	NULL
};
ATTRIBUTE_GROUPS(link);

static struct kobj_type link_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_groups,
};

int sl_sysfs_link_create(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link create (num = %u)", ctl_link->num);

	if (!ctl_link->parent_kobj) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME, "link create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctl_link->kobj, &link_info, ctl_link->parent_kobj, "link");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_policy_create(ctl_link);
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"sl_sysfs_link_policy_create failed [%d]", rtn);
		kobject_put(&ctl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_config_create(ctl_link);
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_config_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctl_link);
		kobject_put(&ctl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_fec_create(ctl_link);
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_fec_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctl_link);
		sl_sysfs_link_config_delete(ctl_link);
		kobject_put(&ctl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_caps_create(ctl_link);
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_caps_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctl_link);
		sl_sysfs_link_config_delete(ctl_link);
		sl_sysfs_link_fec_delete(ctl_link);
		kobject_put(&ctl_link->kobj);
		return rtn;
	}

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"link create (link_kobj = 0x%p)", &ctl_link->kobj);

	return 0;
}

void sl_sysfs_link_delete(struct sl_ctl_link *ctl_link)
{
	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link delete (num = %u)", ctl_link->num);

	if (!ctl_link->parent_kobj)
		return;

	sl_sysfs_link_policy_delete(ctl_link);
	sl_sysfs_link_config_delete(ctl_link);
	sl_sysfs_link_fec_delete(ctl_link);
	sl_sysfs_link_caps_delete(ctl_link);
	kobject_put(&ctl_link->kobj);
}
