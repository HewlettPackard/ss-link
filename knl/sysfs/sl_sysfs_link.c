// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_link_priv.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "hw/sl_core_hw_an.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  state;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_state_get_cmd(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
				   ctrl_link->ctrl_lgrp->num, ctrl_link->num, &state);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"state show (link = 0x%p, state = %u %s)", ctrl_link, state, sl_link_state_str(state));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_link_state_str(state));
}

static ssize_t speed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  state;
	u32                  speed;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_state_get_cmd(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
				   ctrl_link->ctrl_lgrp->num, ctrl_link->num, &state);
	sl_core_link_speed_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
			       ctrl_link->ctrl_lgrp->num, ctrl_link->num, &speed);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"speed show (state = %u %s, speed = 0x%X %s)",
		state, sl_link_state_str(state), speed, sl_lgrp_config_tech_str(speed));

	if (state != SL_LINK_STATE_UP)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_lgrp_config_tech_str(speed));
}

static ssize_t last_up_fail_cause_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  state;
	u64                  up_fail_cause_map;
	char                 cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	up_fail_cause_map = sl_ctrl_link_last_up_fail_cause_map_get(ctrl_link);

	sl_link_down_cause_map_with_info_str(up_fail_cause_map, cause_str, sizeof(cause_str));

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"last up fail cause show (cause_map = 0x%llX %s)",
		up_fail_cause_map, cause_str);

	if (up_fail_cause_map == SL_LINK_DOWN_CAUSE_NONE) {
		sl_ctrl_link_state_get_cmd(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &state);
		if (state == SL_LINK_STATE_UP)
			return scnprintf(buf, PAGE_SIZE, "no-fail\n");
		else
			return scnprintf(buf, PAGE_SIZE, "no-record\n");
	}

	return scnprintf(buf, PAGE_SIZE, "%s\n", cause_str);
}

static ssize_t last_up_fail_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  state;
	u64                  up_fail_cause_map;
	time64_t             up_fail_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_last_up_fail_cause_info_get(ctrl_link, &up_fail_cause_map, &up_fail_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"last up fail time show (cause_map = 0x%llX, time = %lld %ptTt %ptTd)",
		up_fail_cause_map, up_fail_time, &up_fail_time, &up_fail_time);

	if (up_fail_cause_map == SL_LINK_DOWN_CAUSE_NONE) {
		sl_ctrl_link_state_get_cmd(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &state);
		if (state == SL_LINK_STATE_UP)
			return scnprintf(buf, PAGE_SIZE, "no-fail\n");
		else
			return scnprintf(buf, PAGE_SIZE, "no-record\n");
	}

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &up_fail_time, &up_fail_time);
}

static ssize_t last_down_cause_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u64                  down_cause_map;
	time64_t             down_time;
	char                 cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_core_link_last_down_cause_map_info_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &down_cause_map, &down_time);

	sl_link_down_cause_map_with_info_str(down_cause_map, cause_str, sizeof(cause_str));
	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"last down cause show (cause_map = 0x%llX %s)",
		down_cause_map, cause_str);

	return scnprintf(buf, PAGE_SIZE, "%s\n", cause_str);
}

static ssize_t last_down_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u64                  down_cause_map;
	time64_t             down_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_core_link_last_down_cause_map_info_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &down_cause_map, &down_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"last down time show (cause_map = 0x%llX, time = %lld %ptTt %ptTd)",
		down_cause_map, down_time, &down_time, &down_time);

	if (down_cause_map == SL_LINK_DOWN_CAUSE_NONE)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &down_time, &down_time);
}

static ssize_t ccw_warn_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	bool                 is_limit_crossed;
	time64_t             limit_crossed_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_core_link_ccw_warn_limit_crossed_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &is_limit_crossed, &limit_crossed_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"ccw warn limit crossed show (is_limit_crossed = %d %s)", is_limit_crossed,
		is_limit_crossed ? "yes":"no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_limit_crossed ? "yes":"no");
}

static ssize_t ccw_warn_limit_last_crossed_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	bool                 is_limit_crossed;
	time64_t             limit_crossed_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_core_link_ccw_warn_limit_crossed_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &is_limit_crossed, &limit_crossed_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"ccw warn limit last crossed time show (is_limit_crossed = 0x%X, time = %lld %ptTt %ptTd)",
		is_limit_crossed, limit_crossed_time, &limit_crossed_time, &limit_crossed_time);

	if (!is_limit_crossed)
		return scnprintf(buf, PAGE_SIZE, "not-crossed\n");

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &limit_crossed_time, &limit_crossed_time);
}

static ssize_t ucw_warn_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	bool                 is_limit_crossed;
	time64_t             limit_crossed_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_core_link_ucw_warn_limit_crossed_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &is_limit_crossed, &limit_crossed_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"ucw warn limit crossed show (is_limit_crossed = %d %s)", is_limit_crossed,
		is_limit_crossed ? "yes":"no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_limit_crossed ? "yes":"no");
}

static ssize_t ucw_warn_limit_last_crossed_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	bool                 is_limit_crossed;
	time64_t             limit_crossed_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_core_link_ucw_warn_limit_crossed_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &is_limit_crossed, &limit_crossed_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"ucw warn limit last crossed time show (is_limit_crossed = 0x%X, time = %lld %ptTt %ptTd)",
		is_limit_crossed, limit_crossed_time, &limit_crossed_time, &limit_crossed_time);

	if (!is_limit_crossed)
		return scnprintf(buf, PAGE_SIZE, "not-crossed\n");

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &limit_crossed_time, &limit_crossed_time);
}

static ssize_t up_count_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  up_count;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_up_count_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &up_count);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "up count show (up_count = %u)", up_count);

	return scnprintf(buf, PAGE_SIZE, "%u\n", up_count);
}

static ssize_t time_to_link_up_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	s64                  attempt_time_ms;
	s64                  total_time_ms;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_up_clocks_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &attempt_time_ms, &total_time_ms);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"time to link up show (attempt_time = %lldms, total_time = %lldms)",
		attempt_time_ms, total_time_ms);

	return scnprintf(buf, PAGE_SIZE, "%lld\n", attempt_time_ms);
}

static ssize_t total_time_to_link_up_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	s64                  attempt_time_ms;
	s64                  total_time_ms;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_up_clocks_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &attempt_time_ms, &total_time_ms);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"total time to link up show (attempt_time = %lldms, total_time = %lldms)",
		attempt_time_ms, total_time_ms);

	return scnprintf(buf, PAGE_SIZE, "%lld\n", total_time_ms);
}

static ssize_t lp_caps_state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	u32                  lp_caps_state;
	struct sl_ctrl_link *ctrl_link;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_an_lp_caps_state_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &lp_caps_state);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "an lp caps state show (lp_caps_state = %u %s)",
		lp_caps_state, sl_link_an_lp_caps_state_str(lp_caps_state));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_link_an_lp_caps_state_str(lp_caps_state));
}

static ssize_t last_autoneg_fail_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  fail_cause;
	time64_t             fail_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_an_fail_cause_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &fail_cause, &fail_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "last autoneg fail casue show (cause = %u %s)",
		fail_cause, sl_core_link_an_fail_cause_str(fail_cause));

	if (fail_cause == SL_CORE_HW_AN_FAIL_CAUSE_NONE)
		return scnprintf(buf, PAGE_SIZE, "no-fail\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_core_link_an_fail_cause_str(fail_cause));
}

static ssize_t last_autoneg_fail_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  fail_cause;
	time64_t             fail_time;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	sl_ctrl_link_an_fail_cause_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
		ctrl_link->num, &fail_cause, &fail_time);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "last autoneg fail time show (cause = %u %s, time = %lld %ptTt %ptTd)",
		fail_cause, sl_core_link_an_fail_cause_str(fail_cause), fail_time, &fail_time, &fail_time);

	if (fail_cause == SL_CORE_HW_AN_FAIL_CAUSE_NONE)
		return scnprintf(buf, PAGE_SIZE, "no-fail\n");

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &fail_time, &fail_time);
}

static ssize_t info_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	u64                  info_map;
	char                 info_map_str[900];

	ctrl_link = container_of(kobj, struct sl_ctrl_link, kobj);

	rtn = sl_ctrl_link_info_map_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
		ctrl_link->ctrl_lgrp->num, ctrl_link->num, &info_map);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"info map show sl_ctrl_link_info_map_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "no_link\n");
	}

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"info map show (info_map = 0x%llX %s)", info_map, info_map_str);

	return scnprintf(buf, PAGE_SIZE, "%s\n", info_map_str);
}

static struct kobj_attribute link_state                            = __ATTR_RO(state);
static struct kobj_attribute link_speed                            = __ATTR_RO(speed);
static struct kobj_attribute link_last_up_fail_cause_map           = __ATTR_RO(last_up_fail_cause_map);
static struct kobj_attribute link_last_up_fail_time                = __ATTR_RO(last_up_fail_time);
static struct kobj_attribute link_last_down_cause_map              = __ATTR_RO(last_down_cause_map);
static struct kobj_attribute link_last_down_time                   = __ATTR_RO(last_down_time);
static struct kobj_attribute link_ccw_warn_limit_crossed           = __ATTR_RO(ccw_warn_limit_crossed);
static struct kobj_attribute link_ccw_warn_limit_last_crossed_time = __ATTR_RO(ccw_warn_limit_last_crossed_time);
static struct kobj_attribute link_ucw_warn_limit_crossed           = __ATTR_RO(ucw_warn_limit_crossed);
static struct kobj_attribute link_ucw_warn_limit_last_crossed_time = __ATTR_RO(ucw_warn_limit_last_crossed_time);
static struct kobj_attribute link_up_count                         = __ATTR_RO(up_count);
static struct kobj_attribute link_time_to_link_up_ms               = __ATTR_RO(time_to_link_up_ms);
static struct kobj_attribute link_total_time_to_link_up_ms         = __ATTR_RO(total_time_to_link_up_ms);
static struct kobj_attribute link_lp_caps_state                    = __ATTR_RO(lp_caps_state);
static struct kobj_attribute link_last_autoneg_fail_cause          = __ATTR_RO(last_autoneg_fail_cause);
static struct kobj_attribute link_last_autoneg_fail_time           = __ATTR_RO(last_autoneg_fail_time);
static struct kobj_attribute link_info_map                         = __ATTR_RO(info_map);

static struct attribute *link_attrs[] = {
	&link_state.attr,
	&link_speed.attr,
	&link_last_up_fail_cause_map.attr,
	&link_last_up_fail_time.attr,
	&link_last_down_cause_map.attr,
	&link_last_down_time.attr,
	&link_ccw_warn_limit_crossed.attr,
	&link_ccw_warn_limit_last_crossed_time.attr,
	&link_ucw_warn_limit_crossed.attr,
	&link_ucw_warn_limit_last_crossed_time.attr,
	&link_up_count.attr,
	&link_time_to_link_up_ms.attr,
	&link_total_time_to_link_up_ms.attr,
	&link_lp_caps_state.attr,
	&link_last_autoneg_fail_cause.attr,
	&link_last_autoneg_fail_time.attr,
	&link_info_map.attr,
	NULL
};
ATTRIBUTE_GROUPS(link);

static struct kobj_type link_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_groups,
};

int sl_sysfs_link_create(struct sl_ctrl_link *ctrl_link)
{
	int rtn;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link create (num = %u)", ctrl_link->num);

	if (!ctrl_link->parent_kobj) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME, "link create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctrl_link->kobj, &link_info, ctrl_link->parent_kobj, "link");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_policy_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"sl_sysfs_link_policy_create failed [%d]", rtn);
		kobject_put(&ctrl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_config_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_config_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctrl_link);
		kobject_put(&ctrl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_degrade_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_degrade_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctrl_link);
		sl_sysfs_link_config_delete(ctrl_link);
		kobject_put(&ctrl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_fec_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_fec_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctrl_link);
		sl_sysfs_link_config_delete(ctrl_link);
		sl_sysfs_link_degrade_delete(ctrl_link);
		kobject_put(&ctrl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_caps_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_caps_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctrl_link);
		sl_sysfs_link_config_delete(ctrl_link);
		sl_sysfs_link_degrade_delete(ctrl_link);
		sl_sysfs_link_fec_delete(ctrl_link);
		kobject_put(&ctrl_link->kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_counters_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME, "sl_sysfs_link_counters_create failed [%d]", rtn);
		sl_sysfs_link_policy_delete(ctrl_link);
		sl_sysfs_link_config_delete(ctrl_link);
		sl_sysfs_link_degrade_delete(ctrl_link);
		sl_sysfs_link_fec_delete(ctrl_link);
		sl_sysfs_link_caps_delete(ctrl_link);
		kobject_put(&ctrl_link->kobj);
		return rtn;
	}

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"link create (link_kobj = 0x%p)", &ctrl_link->kobj);

	return 0;
}

void sl_sysfs_link_delete(struct sl_ctrl_link *ctrl_link)
{
	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link delete (num = %u)", ctrl_link->num);

	if (!ctrl_link->parent_kobj)
		return;

	sl_sysfs_link_policy_delete(ctrl_link);
	sl_sysfs_link_config_delete(ctrl_link);
	sl_sysfs_link_degrade_delete(ctrl_link);
	sl_sysfs_link_fec_delete(ctrl_link);
	sl_sysfs_link_caps_delete(ctrl_link);
	sl_sysfs_link_counters_delete(ctrl_link);
	kobject_put(&ctrl_link->kobj);
}
