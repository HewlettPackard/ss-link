// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include <linux/hpe/sl/sl_llr.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_core_str.h"
#include "sl_ctrl_llr.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"

#include "data/sl_core_data_llr.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t last_fail_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 llr_fail_cause;
	time64_t            llr_fail_time;

	core_llr = container_of(kobj, struct sl_core_llr, kobj);

	rtn = sl_core_data_llr_last_fail_cause_get(core_llr, &llr_fail_cause, &llr_fail_time);
	if (rtn)
		scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "last fail cause show (cause = %u %s)", llr_fail_cause, sl_core_llr_fail_cause_str(llr_fail_cause));

	if (llr_fail_cause == SL_LLR_FAIL_CAUSE_NONE)
		return scnprintf(buf, PAGE_SIZE, "no-fail\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_core_llr_fail_cause_str(llr_fail_cause));
}

static ssize_t last_fail_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 llr_fail_cause;
	time64_t            llr_fail_time;

	core_llr = container_of(kobj, struct sl_core_llr, kobj);

	rtn = sl_core_data_llr_last_fail_cause_get(core_llr, &llr_fail_cause, &llr_fail_time);
	if (rtn)
		scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "last fail time show (cause = %u %s, time = %lld %ptTt %ptTd)",
		   llr_fail_cause, sl_core_llr_fail_cause_str(llr_fail_cause),
		   llr_fail_time, &llr_fail_time, &llr_fail_time);

	if (llr_fail_cause == SL_LLR_FAIL_CAUSE_NONE)
		return scnprintf(buf, PAGE_SIZE, "no-fail\n");

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &llr_fail_time, &llr_fail_time);
}

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 core_llr_state;
	u32                 llr_state;

	core_llr = container_of(kobj, struct sl_core_llr, kobj);

	rtn = sl_core_data_llr_state_get(core_llr, &core_llr_state);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	llr_state = sl_ctrl_llr_state_from_core_llr_state(core_llr_state);

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "state show (core_llr_state = %u %s, llr_state = %u %s)",
		   core_llr_state, sl_core_llr_state_str(core_llr_state),
		   llr_state, sl_llr_state_str(llr_state));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_llr_state_str(llr_state));
}

static ssize_t info_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u64                 info_map;
	char                info_map_str[900];

	core_llr = container_of(kobj, struct sl_core_llr, kobj);

	rtn = sl_core_data_llr_info_map_get(core_llr, &info_map);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "info map show (info_map = 0x%llX %s)", info_map, info_map_str);

	return scnprintf(buf, PAGE_SIZE, "%s\n", info_map_str);
}

static struct kobj_attribute llr_state           = __ATTR_RO(state);
static struct kobj_attribute llr_last_fail_cause = __ATTR_RO(last_fail_cause);
static struct kobj_attribute llr_last_fail_time  = __ATTR_RO(last_fail_time);
static struct kobj_attribute llr_info_map        = __ATTR_RO(info_map);

static struct attribute *llr_attrs[] = {
	&llr_state.attr,
	&llr_last_fail_cause.attr,
	&llr_last_fail_time.attr,
	&llr_info_map.attr,
	NULL
};
ATTRIBUTE_GROUPS(llr);

static struct kobj_type llr_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = llr_groups,
};

int sl_sysfs_llr_create(struct sl_ctrl_llr *ctrl_llr)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ctrl_llr->ctrl_lgrp->ctrl_ldev->num,
				   ctrl_llr->ctrl_lgrp->num, ctrl_llr->num);

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr create (num = %u)", ctrl_llr->num);

	if (!ctrl_llr->parent_kobj) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&core_llr->kobj, &llr_info, ctrl_llr->parent_kobj, "llr");
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME,
			"llr create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&core_llr->kobj);
		return rtn;
	}

	rtn = sl_sysfs_llr_config_create(ctrl_llr, &core_llr->kobj);
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME,
			"llr config create failed [%d]", rtn);
		kobject_put(&core_llr->kobj);
		return rtn;
	}

	rtn = sl_sysfs_llr_policy_create(core_llr, &core_llr->kobj);
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME,
			"llr policy create failed [%d]", rtn);
		kobject_put(&core_llr->kobj);
		sl_sysfs_llr_config_delete(ctrl_llr);
		return rtn;
	}

	rtn = sl_sysfs_llr_loop_time_create(core_llr, &core_llr->kobj);
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME,
			"llr loop time create failed [%d]", rtn);
		kobject_put(&core_llr->kobj);
		sl_sysfs_llr_config_delete(ctrl_llr);
		sl_sysfs_llr_policy_delete(core_llr);
		return rtn;
	}

	rtn = sl_sysfs_llr_counters_create(ctrl_llr, &core_llr->kobj);
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME, "sl_sysfs_llr_counters_create failed [%d]", rtn);
		kobject_put(&core_llr->kobj);
		sl_sysfs_llr_config_delete(ctrl_llr);
		sl_sysfs_llr_policy_delete(core_llr);
		sl_sysfs_llr_loop_time_delete(core_llr);
		return rtn;
	}

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME,
		"llr create (llr_kobj = 0x%p)", &core_llr->kobj);

	return 0;
}

void sl_sysfs_llr_delete(struct sl_ctrl_llr *ctrl_llr)
{
	struct sl_core_llr *core_llr;

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "delete (num = %u)", ctrl_llr->num);

	if (!ctrl_llr->parent_kobj)
		return;

	core_llr = sl_core_llr_get(ctrl_llr->ctrl_lgrp->ctrl_ldev->num,
				   ctrl_llr->ctrl_lgrp->num, ctrl_llr->num);

	sl_sysfs_llr_counters_delete(ctrl_llr);
	sl_sysfs_llr_loop_time_delete(core_llr);
	sl_sysfs_llr_policy_delete(core_llr);
	sl_sysfs_llr_config_delete(ctrl_llr);
	kobject_put(&core_llr->kobj);
}
