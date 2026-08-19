// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/hpe/sl/sl_fec.h>
#include <linux/time.h>

#include "sl_log.h"
#include "data/sl_ctrl_data_link.h"

#include "sl_sysfs_link_fec_up_history.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

#ifdef CONFIG_SYSFS

static ssize_t fec_up_history_timestamp_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	struct sl_fec_info   record;
	time64_t             record_timestamp;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.up_history_kobj);

	rtn = sl_ctrl_data_link_fec_up_history_get(ctrl_link, num, &record, &record_timestamp);
	if (rtn == -ENOENT)
		return sysfs_emit(buf, "none\n");
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec up history timestamp show (num = %u, time = %lld %ptTt %ptTd)",
		   num, record.ucw, &record_timestamp, &record_timestamp);

	return sysfs_emit(buf, "%ptTt %ptTd\n", &record_timestamp, &record_timestamp);
}

static ssize_t fec_up_history_ucw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	struct sl_fec_info   record;
	time64_t             record_timestamp;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.up_history_kobj);

	rtn = sl_ctrl_data_link_fec_up_history_get(ctrl_link, num, &record, &record_timestamp);
	if (rtn == -ENOENT)
		return sysfs_emit(buf, "none\n");
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec up history ucw show (num = %u, ucw = %llu)", num, record.ucw);

	return sysfs_emit(buf, "%llu\n", record.ucw);
}

static ssize_t fec_up_history_ccw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	struct sl_fec_info   record;
	time64_t             record_timestamp;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.up_history_kobj);

	rtn = sl_ctrl_data_link_fec_up_history_get(ctrl_link, num, &record, &record_timestamp);
	if (rtn == -ENOENT)
		return sysfs_emit(buf, "none\n");
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec up history ccw show (num = %u, ccw = %llu)", num, record.ccw);

	return sysfs_emit(buf, "%llu\n", record.ccw);
}

static ssize_t fec_up_history_gcw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	struct sl_fec_info   record;
	time64_t             record_timestamp;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.up_history_kobj);

	rtn = sl_ctrl_data_link_fec_up_history_get(ctrl_link, num, &record, &record_timestamp);
	if (rtn == -ENOENT)
		return sysfs_emit(buf, "none\n");
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec up history gcw show (num = %u, gcw = %llu)", num, record.gcw);

	return sysfs_emit(buf, "%llu\n", record.gcw);
}

static ssize_t fec_up_history_period_ms_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	struct sl_ctrl_link *ctrl_link;
	int                  rtn;
	struct sl_fec_info   record;
	time64_t             record_timestamp;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.up_history_kobj);

	rtn = sl_ctrl_data_link_fec_up_history_get(ctrl_link, num, &record, &record_timestamp);
	if (rtn == -ENOENT)
		return sysfs_emit(buf, "none\n");
	if (rtn)
		return sysfs_emit(buf, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "fec up history period_ms show (num = %u, period_ms = %u)", num, record.period_ms);

	return sysfs_emit(buf, "%u\n", record.period_ms);
}

#define fec_up_history_timestamp(_num)						\
	static inline ssize_t timestamp_##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{								\
		return fec_up_history_timestamp_show(kobj, kattr, buf, (_num)); \
	}								\
	static struct kobj_attribute fec_up_history_timestamp_##_num = __ATTR_RO(timestamp_##_num)

#define fec_up_history_ucw(_num)						\
	static inline ssize_t ucw_##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{								\
		return fec_up_history_ucw_show(kobj, kattr, buf, (_num)); \
	}								\
	static struct kobj_attribute fec_up_history_ucw_##_num = __ATTR_RO(ucw_##_num)

#define fec_up_history_ccw(_num)						\
	static inline ssize_t ccw_##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{								\
		return fec_up_history_ccw_show(kobj, kattr, buf, (_num)); \
	}								\
	static struct kobj_attribute fec_up_history_ccw_##_num = __ATTR_RO(ccw_##_num)

#define fec_up_history_gcw(_num)						\
	static inline ssize_t gcw_##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{								\
		return fec_up_history_gcw_show(kobj, kattr, buf, (_num)); \
	}								\
	static struct kobj_attribute fec_up_history_gcw_##_num = __ATTR_RO(gcw_##_num)

#define fec_up_history_period_ms(_num)					\
	static inline ssize_t period_ms_##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{								\
		return fec_up_history_period_ms_show(kobj, kattr, buf, (_num)); \
	}								\
	static struct kobj_attribute fec_up_history_period_ms_##_num = __ATTR_RO(period_ms_##_num)

fec_up_history_timestamp(0);
fec_up_history_timestamp(1);
fec_up_history_timestamp(2);
fec_up_history_timestamp(3);
fec_up_history_timestamp(4);
fec_up_history_timestamp(5);
fec_up_history_timestamp(6);
fec_up_history_timestamp(7);
fec_up_history_timestamp(8);
fec_up_history_timestamp(9);

fec_up_history_ucw(0);
fec_up_history_ucw(1);
fec_up_history_ucw(2);
fec_up_history_ucw(3);
fec_up_history_ucw(4);
fec_up_history_ucw(5);
fec_up_history_ucw(6);
fec_up_history_ucw(7);
fec_up_history_ucw(8);
fec_up_history_ucw(9);

fec_up_history_ccw(0);
fec_up_history_ccw(1);
fec_up_history_ccw(2);
fec_up_history_ccw(3);
fec_up_history_ccw(4);
fec_up_history_ccw(5);
fec_up_history_ccw(6);
fec_up_history_ccw(7);
fec_up_history_ccw(8);
fec_up_history_ccw(9);

fec_up_history_gcw(0);
fec_up_history_gcw(1);
fec_up_history_gcw(2);
fec_up_history_gcw(3);
fec_up_history_gcw(4);
fec_up_history_gcw(5);
fec_up_history_gcw(6);
fec_up_history_gcw(7);
fec_up_history_gcw(8);
fec_up_history_gcw(9);

fec_up_history_period_ms(0);
fec_up_history_period_ms(1);
fec_up_history_period_ms(2);
fec_up_history_period_ms(3);
fec_up_history_period_ms(4);
fec_up_history_period_ms(5);
fec_up_history_period_ms(6);
fec_up_history_period_ms(7);
fec_up_history_period_ms(8);
fec_up_history_period_ms(9);

static struct attribute *fec_up_history_attrs[] = {
	&fec_up_history_timestamp_0.attr,
	&fec_up_history_ucw_0.attr,
	&fec_up_history_ccw_0.attr,
	&fec_up_history_gcw_0.attr,
	&fec_up_history_period_ms_0.attr,
	&fec_up_history_timestamp_1.attr,
	&fec_up_history_ucw_1.attr,
	&fec_up_history_ccw_1.attr,
	&fec_up_history_gcw_1.attr,
	&fec_up_history_period_ms_1.attr,
	&fec_up_history_timestamp_2.attr,
	&fec_up_history_ucw_2.attr,
	&fec_up_history_ccw_2.attr,
	&fec_up_history_gcw_2.attr,
	&fec_up_history_period_ms_2.attr,
	&fec_up_history_timestamp_3.attr,
	&fec_up_history_ucw_3.attr,
	&fec_up_history_ccw_3.attr,
	&fec_up_history_gcw_3.attr,
	&fec_up_history_period_ms_3.attr,
	&fec_up_history_timestamp_4.attr,
	&fec_up_history_ucw_4.attr,
	&fec_up_history_ccw_4.attr,
	&fec_up_history_gcw_4.attr,
	&fec_up_history_period_ms_4.attr,
	&fec_up_history_timestamp_5.attr,
	&fec_up_history_ucw_5.attr,
	&fec_up_history_ccw_5.attr,
	&fec_up_history_gcw_5.attr,
	&fec_up_history_period_ms_5.attr,
	&fec_up_history_timestamp_6.attr,
	&fec_up_history_ucw_6.attr,
	&fec_up_history_ccw_6.attr,
	&fec_up_history_gcw_6.attr,
	&fec_up_history_period_ms_6.attr,
	&fec_up_history_timestamp_7.attr,
	&fec_up_history_ucw_7.attr,
	&fec_up_history_ccw_7.attr,
	&fec_up_history_gcw_7.attr,
	&fec_up_history_period_ms_7.attr,
	&fec_up_history_timestamp_8.attr,
	&fec_up_history_ucw_8.attr,
	&fec_up_history_ccw_8.attr,
	&fec_up_history_gcw_8.attr,
	&fec_up_history_period_ms_8.attr,
	&fec_up_history_timestamp_9.attr,
	&fec_up_history_ucw_9.attr,
	&fec_up_history_ccw_9.attr,
	&fec_up_history_gcw_9.attr,
	&fec_up_history_period_ms_9.attr,
	NULL
};
ATTRIBUTE_GROUPS(fec_up_history);

static struct kobj_type fec_up_history_type = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = fec_up_history_groups,
};
#endif /* CONFIG_SYSFS */

int sl_sysfs_link_fec_up_history_create(struct sl_ctrl_link *ctrl_link)
{
#ifdef CONFIG_SYSFS
	int rtn;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link fec up history create");

	rtn = kobject_init_and_add(&ctrl_link->fec.up_history_kobj, &fec_up_history_type,
				   &ctrl_link->fec.kobj, "up_history");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			   "link fec up history create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->fec.up_history_kobj);
		return rtn;
	}

	return 0;
#else /* CONFIG_SYSFS */
	return 0;
#endif /* CONFIG_SYSFS */
}

void sl_sysfs_link_fec_up_history_delete(struct sl_ctrl_link *ctrl_link)
{
#ifdef CONFIG_SYSFS
	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		   "link fec up history delete (link_num = %u)", ctrl_link->num);

	kobject_put(&ctrl_link->fec.up_history_kobj);
#endif /* CONFIG_SYSFS */
}
