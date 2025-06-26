// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include <linux/sl_llr.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_llr.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_core_llr.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t last_fail_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr *ctl_llr;
	u32                llr_fail_cause;
	time64_t           llr_fail_time;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, kobj);

	sl_core_llr_last_fail_cause_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num,
		ctl_llr->num, &llr_fail_cause, &llr_fail_time);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"last fail cause show (cause = %u %s)", llr_fail_cause, sl_core_llr_fail_cause_str(llr_fail_cause));

	if (llr_fail_cause == SL_LLR_FAIL_CAUSE_NONE)
		return scnprintf(buf, PAGE_SIZE, "no-fail\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_core_llr_fail_cause_str(llr_fail_cause));
}

static ssize_t last_fail_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr *ctl_llr;
	u32                llr_fail_cause;
	time64_t           llr_fail_time;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, kobj);

	sl_core_llr_last_fail_cause_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num,
		ctl_llr->num, &llr_fail_cause, &llr_fail_time);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"last fail time show (cause = %u %s, time = %lld %ptTt %ptTd)",
		llr_fail_cause, sl_core_llr_fail_cause_str(llr_fail_cause),
		llr_fail_time, &llr_fail_time, &llr_fail_time);

	if (llr_fail_cause == SL_LLR_FAIL_CAUSE_NONE)
		return scnprintf(buf, PAGE_SIZE, "no-fail\n");

	return scnprintf(buf, PAGE_SIZE, "%ptTt %ptTd\n", &llr_fail_time, &llr_fail_time);
}

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr *ctl_llr;
	u32                state;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, kobj);

	sl_ctl_llr_state_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num, &state);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"state show (llr = 0x%p, link = 0x%p, state = %u)", ctl_llr, ctl_llr, state);

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_llr_state_str(state));
}

static struct kobj_attribute llr_state           = __ATTR_RO(state);
static struct kobj_attribute llr_last_fail_cause = __ATTR_RO(last_fail_cause);
static struct kobj_attribute llr_last_fail_time  = __ATTR_RO(last_fail_time);

static struct attribute *llr_attrs[] = {
	&llr_state.attr,
	&llr_last_fail_cause.attr,
	&llr_last_fail_time.attr,
	NULL
};
ATTRIBUTE_GROUPS(llr);

static struct kobj_type llr_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = llr_groups,
};

int sl_sysfs_llr_create(struct sl_ctl_llr *ctl_llr)
{
	int rtn;

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "llr create (num = %u)", ctl_llr->num);

	if (!ctl_llr->parent_kobj) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME, "llr create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctl_llr->kobj, &llr_info, ctl_llr->parent_kobj, "llr");
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_llr->kobj);
		return rtn;
	}

	rtn = sl_sysfs_llr_config_create(ctl_llr);
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr config create failed [%d]", rtn);
		kobject_put(&ctl_llr->kobj);
		return rtn;
	}

	rtn = sl_sysfs_llr_policy_create(ctl_llr);
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr policy create failed [%d]", rtn);
		kobject_put(&ctl_llr->kobj);
		kobject_put(&ctl_llr->config_kobj);
		return rtn;
	}

	rtn = sl_sysfs_llr_loop_time_create(ctl_llr);
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr loop time create failed [%d]", rtn);
		kobject_put(&ctl_llr->kobj);
		kobject_put(&ctl_llr->config_kobj);
		kobject_put(&ctl_llr->policy_kobj);
		return rtn;
	}

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"llr create (llr_kobj = 0x%p)", &ctl_llr->kobj);

	return 0;
}

void sl_sysfs_llr_delete(struct sl_ctl_llr *ctl_llr)
{
	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "delete (num = %u)", ctl_llr->num);

	if (!ctl_llr->parent_kobj)
		return;

	sl_sysfs_llr_loop_time_delete(ctl_llr);
	sl_sysfs_llr_policy_delete(ctl_llr);
	sl_sysfs_llr_config_delete(ctl_llr);
	kobject_put(&ctl_llr->kobj);
}
