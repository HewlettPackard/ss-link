// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_llr.h"
#include "sl_ctl_llr.h"
#include "sl_core_llr.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t llr_loop_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	struct sl_ctl_llr  *ctl_llr;
	struct sl_core_llr *core_llr;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, loop_time_kobj);
	core_llr = sl_core_llr_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num);

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		"loop time show (llr = 0x%p, loop time %u = %lluns)",
		core_llr, num, core_llr->loop_time[num]);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", core_llr->loop_time[num]);
}

#define llr_loop_time(_num)                                                                                        \
	static inline ssize_t time_##_num##_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                          \
		return llr_loop_time_show(kobj, kattr, buf, (_num));                                               \
	}                                                                                                          \
	static struct kobj_attribute llr_loop_time_##_num = __ATTR_RO(time_##_num##_ns)

llr_loop_time(0);
llr_loop_time(1);
llr_loop_time(2);
llr_loop_time(3);
llr_loop_time(4);
llr_loop_time(5);
llr_loop_time(6);
llr_loop_time(7);
llr_loop_time(8);
llr_loop_time(9);

#define sl_sysfs_llr_loop_time_file(_num)                                                      \
	do {                                                                                   \
		rtn = sysfs_create_file(&ctl_llr->loop_time_kobj, &llr_loop_time_##_num.attr); \
		if (rtn) {                                                                     \
			sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,                               \
				"llr loop time create file failed [%d]", rtn);                 \
			goto out;                                                              \
		}                                                                              \
	} while (0)

static ssize_t calc_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr  *ctl_llr;
	struct sl_llr_data  llr_data;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, loop_time_kobj);
	sl_core_llr_data_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num, &llr_data);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"calc show (llr = 0x%p, calc = %lluns)", ctl_llr, llr_data.loop.calculated);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", llr_data.loop.calculated);
}

static ssize_t min_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr  *ctl_llr;
	struct sl_llr_data  llr_data;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, loop_time_kobj);
	sl_core_llr_data_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num, &llr_data);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"min show (llr = 0x%p, min = %lluns)", ctl_llr, llr_data.loop.min);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", llr_data.loop.min);
}

static ssize_t max_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr  *ctl_llr;
	struct sl_llr_data  llr_data;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, loop_time_kobj);
	sl_core_llr_data_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num, &llr_data);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"max show (llr = 0x%p, max = %lluns)", ctl_llr, llr_data.loop.max);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", llr_data.loop.max);
}

static ssize_t average_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr  *ctl_llr;
	struct sl_llr_data  llr_data;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, loop_time_kobj);
	sl_core_llr_data_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num, &llr_data);

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME,
		"average show (llr = 0x%p, average = %lluns)", ctl_llr, llr_data.loop.average);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", llr_data.loop.average);
}

static struct kobj_attribute llr_loop_calc    = __ATTR_RO(calc_ns);
static struct kobj_attribute llr_loop_min     = __ATTR_RO(min_ns);
static struct kobj_attribute llr_loop_max     = __ATTR_RO(max_ns);
static struct kobj_attribute llr_loop_average = __ATTR_RO(average_ns);

static struct attribute *llr_loop_time_attrs[] = {
	NULL
};
ATTRIBUTE_GROUPS(llr_loop_time);

static struct kobj_type llr_loop_time = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = llr_loop_time_groups,
};

int sl_sysfs_llr_loop_time_create(struct sl_ctl_llr *ctl_llr)
{
	int rtn;

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "llr loop time create (num = %u)", ctl_llr->num);

	rtn = kobject_init_and_add(&ctl_llr->loop_time_kobj, &llr_loop_time, &ctl_llr->kobj, "loop");
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr loop time create kobject_init_and_add failed [%d]", rtn);
		goto out;
	}

	BUILD_BUG_ON(SL_CORE_LLR_MAX_LOOP_TIME_COUNT < 10);

	sl_sysfs_llr_loop_time_file(0);
	sl_sysfs_llr_loop_time_file(1);
	sl_sysfs_llr_loop_time_file(2);
	sl_sysfs_llr_loop_time_file(3);
	sl_sysfs_llr_loop_time_file(4);
	sl_sysfs_llr_loop_time_file(5);
	sl_sysfs_llr_loop_time_file(6);
	sl_sysfs_llr_loop_time_file(7);
	sl_sysfs_llr_loop_time_file(8);
	sl_sysfs_llr_loop_time_file(9);

	rtn = sysfs_create_file(&ctl_llr->loop_time_kobj, &llr_loop_calc.attr);
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr loop time create file failed [%d]", rtn);
		goto out;
	}

	rtn = sysfs_create_file(&ctl_llr->loop_time_kobj, &llr_loop_min.attr);
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr loop time create file failed [%d]", rtn);
		goto out;
	}

	rtn = sysfs_create_file(&ctl_llr->loop_time_kobj, &llr_loop_max.attr);
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr loop time create file failed [%d]", rtn);
		goto out;
	}

	rtn = sysfs_create_file(&ctl_llr->loop_time_kobj, &llr_loop_average.attr);
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr loop time create file failed [%d]", rtn);
		goto out;
	}

	return 0;
out:
	kobject_put(&ctl_llr->loop_time_kobj);
	return rtn;
}

void sl_sysfs_llr_loop_time_delete(struct sl_ctl_llr *ctl_llr)
{
	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "llr loop time delete (num = %u)", ctl_llr->num);

	kobject_put(&ctl_llr->loop_time_kobj);
}
