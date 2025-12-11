// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "data/sl_core_data_llr.h"

#include "sl_sysfs.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t llr_loop_time_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u64                 loop_time[SL_CORE_LLR_MAX_LOOP_TIME_COUNT];

	core_llr = container_of(kobj, struct sl_core_llr, loop_time_kobj);

	rtn = sl_core_data_llr_loop_time_get(core_llr, loop_time);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "loop time show (loop_time[%u] = %lluns)", num, loop_time[num]);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", loop_time[num]);
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

#define sl_sysfs_llr_loop_time_file(_num)                                                       \
	do {                                                                                    \
		rtn = sysfs_create_file(&core_llr->loop_time_kobj, &llr_loop_time_##_num.attr); \
		if (rtn) {                                                                      \
			sl_log_err(core_llr, LOG_BLOCK, LOG_NAME,                               \
				"llr loop time create file failed [%d]", rtn);                  \
			goto out;                                                               \
		}                                                                               \
	} while (0)

static ssize_t calc_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u64                 calculated_ns;

	core_llr = container_of(kobj, struct sl_core_llr, loop_time_kobj);

	rtn = sl_core_data_llr_loop_calculated_ns_get(core_llr, &calculated_ns);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "calc show (calculated_ns = %lluns)", calculated_ns);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", calculated_ns);
}

static ssize_t min_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u64                 min_ns;

	core_llr = container_of(kobj, struct sl_core_llr, loop_time_kobj);

	rtn = sl_core_data_llr_loop_min_ns_get(core_llr, &min_ns);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "min show (min_ns = %lluns)", min_ns);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", min_ns);
}

static ssize_t max_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u64                 max_ns;

	core_llr = container_of(kobj, struct sl_core_llr, loop_time_kobj);

	rtn = sl_core_data_llr_loop_max_ns_get(core_llr, &max_ns);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "max show (max_ns = %lluns)", max_ns);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", max_ns);
}

static ssize_t average_ns_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u64                 average_ns;

	core_llr = container_of(kobj, struct sl_core_llr, loop_time_kobj);

	rtn = sl_core_data_llr_loop_average_ns_get(core_llr, &average_ns);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME,
		   "average show (average_ns = %lluns)", average_ns);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", average_ns);
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

int sl_sysfs_llr_loop_time_create(struct sl_core_llr *core_llr, struct kobject *parent_kobj)
{
	int rtn;

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME, "llr loop time create (num = %u)", core_llr->num);

	rtn = kobject_init_and_add(&core_llr->loop_time_kobj, &llr_loop_time, parent_kobj, "loop");
	if (rtn) {
		sl_log_err(core_llr, LOG_BLOCK, LOG_NAME,
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

	rtn = sysfs_create_file(&core_llr->loop_time_kobj, &llr_loop_calc.attr);
	if (rtn) {
		sl_log_err(core_llr, LOG_BLOCK, LOG_NAME,
			   "llr loop time create file failed [%d]", rtn);
		goto out;
	}

	rtn = sysfs_create_file(&core_llr->loop_time_kobj, &llr_loop_min.attr);
	if (rtn) {
		sl_log_err(core_llr, LOG_BLOCK, LOG_NAME,
			   "llr loop time create file failed [%d]", rtn);
		goto out;
	}

	rtn = sysfs_create_file(&core_llr->loop_time_kobj, &llr_loop_max.attr);
	if (rtn) {
		sl_log_err(core_llr, LOG_BLOCK, LOG_NAME,
			   "llr loop time create file failed [%d]", rtn);
		goto out;
	}

	rtn = sysfs_create_file(&core_llr->loop_time_kobj, &llr_loop_average.attr);
	if (rtn) {
		sl_log_err(core_llr, LOG_BLOCK, LOG_NAME,
			   "llr loop time create file failed [%d]", rtn);
		goto out;
	}

	return 0;
out:
	kobject_put(&core_llr->loop_time_kobj);
	return rtn;
}

void sl_sysfs_llr_loop_time_delete(struct sl_core_llr *core_llr)
{
	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME, "llr loop time delete (num = %u)", core_llr->num);

	kobject_put(&core_llr->loop_time_kobj);
}
