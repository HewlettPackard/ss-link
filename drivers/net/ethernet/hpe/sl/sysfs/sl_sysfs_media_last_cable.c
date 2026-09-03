// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/sysfs.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_core_str.h"
#include "sl_core_lgrp.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "data/sl_media_data_jack.h"
#include "base/sl_media_eeprom.h"
#include "sl_sysfs_media_last_cable.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

#ifdef CONFIG_SYSFS
static ssize_t last_cable_time(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	struct sl_media_lgrp              *media_lgrp;
	struct sl_media_cable_insert_entry cable_insert_entry;
	int		                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, last_cable_insert_kobj);

	rtn = sl_media_data_jack_last_cable_insert_get(media_lgrp->media_jack, num, &cable_insert_entry);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	if (!cable_insert_entry.timestamp)
		return sysfs_emit(buf, "none\n");

	sl_log_dbg(media_lgrp, LOG_BLOCK, LOG_NAME,
		   "last cable time show (time = %lld %ptTd %ptTt)", cable_insert_entry.timestamp,
		   &cable_insert_entry.timestamp, &cable_insert_entry.timestamp);

	return sysfs_emit(buf, "%ptTd %ptTt\n", &cable_insert_entry.timestamp, &cable_insert_entry.timestamp);
}

#define media_last_cable_time(_num)                                                                             \
	static inline ssize_t time_##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                       \
		return last_cable_time(kobj, kattr, buf, (_num));                                               \
	}                                                                                                       \
	static struct kobj_attribute media_last_cable_time_##_num = __ATTR_RO(time_##_num)

media_last_cable_time(0);
media_last_cable_time(1);
media_last_cable_time(2);
media_last_cable_time(3);
media_last_cable_time(4);
media_last_cable_time(5);
media_last_cable_time(6);
media_last_cable_time(7);
media_last_cable_time(8);
media_last_cable_time(9);

static struct attribute *media_last_cable_attrs[] = {
	&media_last_cable_time_0.attr,
	&media_last_cable_time_1.attr,
	&media_last_cable_time_2.attr,
	&media_last_cable_time_3.attr,
	&media_last_cable_time_4.attr,
	&media_last_cable_time_5.attr,
	&media_last_cable_time_6.attr,
	&media_last_cable_time_7.attr,
	&media_last_cable_time_8.attr,
	&media_last_cable_time_9.attr,
	NULL,
};
ATTRIBUTE_GROUPS(media_last_cable);

static struct kobj_type media_last_cable_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = media_last_cable_groups,
};

static ssize_t vendor_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct sl_media_lgrp              *media_lgrp;
	struct sl_media_cable_insert_entry cable_insert_entry;
	struct media_cable_entry          *entry;
	int                                rtn;
	u8                                 num;

	entry = container_of(kobj, struct media_cable_entry, kobj);
	num = entry->index;

	media_lgrp = entry->parent;

	rtn = sl_media_data_jack_last_cable_insert_get(media_lgrp->media_jack, num, &cable_insert_entry);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	if (!cable_insert_entry.timestamp)
		return sysfs_emit(buf, "none\n");

	sl_log_dbg(media_lgrp, LOG_BLOCK, LOG_NAME,
		   "vendor show (num = %u, vendor = %s)",
		   num, sl_media_vendor_str(cable_insert_entry.cable.vendor));

	return sysfs_emit(buf, "%s\n", sl_media_vendor_str(cable_insert_entry.cable.vendor));
}

static ssize_t type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct sl_media_lgrp              *media_lgrp;
	struct sl_media_cable_insert_entry cable_insert_entry;
	struct media_cable_entry          *entry;
	int                                rtn;
	u8                                 num;

	entry = container_of(kobj, struct media_cable_entry, kobj);
	num = entry->index;

	media_lgrp = entry->parent;

	rtn = sl_media_data_jack_last_cable_insert_get(media_lgrp->media_jack, num, &cable_insert_entry);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	if (!cable_insert_entry.timestamp)
		return sysfs_emit(buf, "none\n");

	sl_log_dbg(media_lgrp, LOG_BLOCK, LOG_NAME,
		   "type show (num = %u, type = %s)", num, sl_media_type_str(cable_insert_entry.cable.type));

	return sysfs_emit(buf, "%s\n", sl_media_type_str(cable_insert_entry.cable.type));

}

static ssize_t serial_num_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct sl_media_lgrp              *media_lgrp;
	struct sl_media_cable_insert_entry cable_insert_entry;
	struct media_cable_entry          *entry;
	int                                rtn;
	u8                                 num;

	entry = container_of(kobj, struct media_cable_entry, kobj);
	num = entry->index;

	media_lgrp = entry->parent;

	rtn = sl_media_data_jack_last_cable_insert_get(media_lgrp->media_jack, num, &cable_insert_entry);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	if (!cable_insert_entry.timestamp)
		return sysfs_emit(buf, "none\n");

	sl_log_dbg(media_lgrp, LOG_BLOCK, LOG_NAME,
		   "serial num show (num = %u, serial_num = %s)", num,
		   cable_insert_entry.cable.serial_num_str);

	return sysfs_emit(buf, "%s\n", cable_insert_entry.cable.serial_num_str);

}

static struct kobj_attribute media_last_cable_vendor     = __ATTR_RO(vendor);
static struct kobj_attribute media_last_cable_type       = __ATTR_RO(type);
static struct kobj_attribute media_last_cable_serial_num = __ATTR_RO(serial_num);

static struct attribute *media_cable_attrs[] = {
	&media_last_cable_vendor.attr,
	&media_last_cable_type.attr,
	&media_last_cable_serial_num.attr,
	NULL,
};
ATTRIBUTE_GROUPS(media_cable);

static struct kobj_type media_cable_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = media_cable_groups,
};

#endif /* CONFIG_SYSFS */

int sl_sysfs_media_last_cable_create(struct sl_ctrl_lgrp *ctrl_lgrp, struct sl_media_lgrp *media_lgrp)
{
#ifdef CONFIG_SYSFS
	struct media_cable_entry *entry;
	int    rtn;
	int    i;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media last cable create");

	rtn = kobject_init_and_add(&media_lgrp->last_cable_insert_kobj, &media_last_cable_info,
				   &media_lgrp->kobj, "last_cable_insert");
	if (rtn) {
		kobject_put(&media_lgrp->last_cable_insert_kobj);
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media last cable insert kobject_init_and_add failed [%d]",
			   rtn);
		return rtn;
	}

	for (i = 0; i < SL_MEDIA_JACK_LAST_CABLE_INSERT_NUM_ENTRIES; ++i) {
		entry = &media_lgrp->last_cable_insert_cables_kobj[i];
		entry->index = i;
		entry->parent = media_lgrp;

		rtn = kobject_init_and_add(&entry->kobj, &media_cable_info, &media_lgrp->last_cable_insert_kobj,
					   "cable_%d", i);
		if (rtn) {
			sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				   "media cable_%d insert kobject_init_and_add failed [%d]", i, rtn);
			while (i >= 0) {
				kobject_put(&media_lgrp->last_cable_insert_cables_kobj[i].kobj);
				i--;
			}
			return rtn;
		}
	}
	return 0;
#else /* CONFIG_SYSFS */
	return 0;
#endif /* CONFIG_SYSFS */
}
