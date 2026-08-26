// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025-2026 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "sl_sysfs.h"
#include "sl_log.h"
#include "sl_media_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_media_counters.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

#ifdef CONFIG_SYSFS
static ssize_t cause_eeprom_format_unsupported_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_EEPROM_FORMAT_UNSUPPORTED, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause eeprom format unsupported show (counter = %u)",
		   counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_eeprom_vendor_unsupported_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_EEPROM_VENDOR_UNSUPPORTED, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause eeprom vendor unsupported show (counter = %u)",
		   counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_eeprom_jack_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_EEPROM_JACK_IO, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause eeprom jack io show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_jack_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_JACK_GET, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause jack get show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_jack_status_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_JACK_STATUS_GET, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause jack status get show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_cable_setup_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_CABLE_SETUP, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause cable setup show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_active_cable_setup_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_ACTIVE_CABLE_SETUP, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause active cable setup show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_serdes_settings_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SERDES_SETTINGS_GET, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause serdes settings get show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_media_attr_set_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_MEDIA_ATTR_SET, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause media attr set show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_high_power_set_jack_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_HIGH_POWER_SET_JACK_IO, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause high power set io show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_shift_down_jack_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift down jack io show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_shift_down_jack_io_low_power_set_show(struct kobject *kobj,
							   struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET,
					      &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift down jack io low power set show (counter = %u)",
		   counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_shift_down_jack_io_high_power_set_show(struct kobject *kobj,
							    struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack,
					      MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift down jack io high power set show (counter = %u)",
		   counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_shift_up_jack_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift up jack io show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_shift_up_jack_io_low_power_set_show(struct kobject *kobj, struct kobj_attribute *kattr,
							 char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET,
					      &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift up jack io low power set show (counter = %u)",
		   counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_shift_up_jack_io_high_power_set_show(struct kobject *kobj, struct kobj_attribute *kattr,
							  char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET,
					      &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift up jack io high power set show (counter = %u)",
		   counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_shift_state_jack_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SHIFT_STATE_JACK_IO, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift state jack io show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_hot_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_HOT, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause hot show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t cause_warm_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_WARM, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause warm show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t temperature_state_cold_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_temp_state_counter_get(media_lgrp->media_jack, MEDIA_TEMP_STATE_COLD, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media temperature state cold show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t temperature_state_warm_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_temp_state_counter_get(media_lgrp->media_jack, MEDIA_TEMP_STATE_WARM, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media temperature state warm show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t temperature_state_hot_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_temp_state_counter_get(media_lgrp->media_jack, MEDIA_TEMP_STATE_HOT, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media temperature state hot show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t temperature_state_unknown_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_temp_state_counter_get(media_lgrp->media_jack, MEDIA_TEMP_STATE_UNKNOWN_IO, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media temperature state unknown io show (counter = %u)", counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static ssize_t temperature_state_unknown_slope_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_temp_state_counter_get(media_lgrp->media_jack, MEDIA_TEMP_STATE_UNKNOWN_SLOPE, &counter);
	if (rtn)
		return sysfs_emit(buf, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media temperature state unknown slope show (counter = %u)",
		   counter);

	return sysfs_emit(buf, "%u\n", counter);
}

static struct kobj_attribute media_cause_eeprom_format_unsupported         = __ATTR_RO(cause_eeprom_format_unsupported);
static struct kobj_attribute media_cause_eeprom_vendor_unsupported         = __ATTR_RO(cause_eeprom_vendor_unsupported);
static struct kobj_attribute media_cause_eeprom_jack_io                    = __ATTR_RO(cause_eeprom_jack_io);
static struct kobj_attribute media_cause_jack_get                          = __ATTR_RO(cause_jack_get);
static struct kobj_attribute media_cause_jack_status_get                   = __ATTR_RO(cause_jack_status_get);
static struct kobj_attribute media_cause_cable_setup                       = __ATTR_RO(cause_cable_setup);
static struct kobj_attribute media_cause_active_cable_setup                = __ATTR_RO(cause_active_cable_setup);
static struct kobj_attribute media_cause_serdes_settings_get               = __ATTR_RO(cause_serdes_settings_get);
static struct kobj_attribute media_cause_media_attr_set                    = __ATTR_RO(cause_media_attr_set);
static struct kobj_attribute media_cause_high_power_set_jack_io            = __ATTR_RO(cause_high_power_set_jack_io);
static struct kobj_attribute media_cause_shift_down_jack_io                = __ATTR_RO(cause_shift_down_jack_io);
static struct kobj_attribute media_cause_shift_down_jack_io_low_power_set  = __ATTR_RO(cause_shift_down_jack_io_low_power_set);
static struct kobj_attribute media_cause_shift_down_jack_io_high_power_set = __ATTR_RO(cause_shift_down_jack_io_high_power_set);
static struct kobj_attribute media_cause_shift_up_jack_io                  = __ATTR_RO(cause_shift_up_jack_io);
static struct kobj_attribute media_cause_shift_up_jack_io_low_power_set    = __ATTR_RO(cause_shift_up_jack_io_low_power_set);
static struct kobj_attribute media_cause_shift_up_jack_io_high_power_set   = __ATTR_RO(cause_shift_up_jack_io_high_power_set);
static struct kobj_attribute media_cause_shift_state_jack_io               = __ATTR_RO(cause_shift_state_jack_io);
static struct kobj_attribute media_cause_hot                               = __ATTR_RO(cause_hot);
static struct kobj_attribute media_cause_warm                              = __ATTR_RO(cause_warm);
static struct kobj_attribute media_temperature_state_cold                  = __ATTR_RO(temperature_state_cold);
static struct kobj_attribute media_temperature_state_warm                  = __ATTR_RO(temperature_state_warm);
static struct kobj_attribute media_temperature_state_hot                   = __ATTR_RO(temperature_state_hot);
static struct kobj_attribute media_temperature_state_unknown_io            = __ATTR_RO(temperature_state_unknown_io);
static struct kobj_attribute media_temperature_state_unknown_slope         = __ATTR_RO(temperature_state_unknown_slope);

static struct attribute *media_counters_attrs[] = {
	&media_cause_eeprom_format_unsupported.attr,
	&media_cause_eeprom_vendor_unsupported.attr,
	&media_cause_eeprom_jack_io.attr,
	&media_cause_jack_get.attr,
	&media_cause_jack_status_get.attr,
	&media_cause_cable_setup.attr,
	&media_cause_active_cable_setup.attr,
	&media_cause_serdes_settings_get.attr,
	&media_cause_media_attr_set.attr,
	&media_cause_high_power_set_jack_io.attr,
	&media_cause_shift_down_jack_io.attr,
	&media_cause_shift_down_jack_io_low_power_set.attr,
	&media_cause_shift_down_jack_io_high_power_set.attr,
	&media_cause_shift_up_jack_io.attr,
	&media_cause_shift_up_jack_io_low_power_set.attr,
	&media_cause_shift_up_jack_io_high_power_set.attr,
	&media_cause_shift_state_jack_io.attr,
	&media_cause_hot.attr,
	&media_cause_warm.attr,
	&media_temperature_state_cold.attr,
	&media_temperature_state_warm.attr,
	&media_temperature_state_hot.attr,
	&media_temperature_state_unknown_io.attr,
	&media_temperature_state_unknown_slope.attr,
	NULL
};
ATTRIBUTE_GROUPS(media_counters);

static struct kobj_type media_counters = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = media_counters_groups,
};
#endif /* CONFIG_SYSFS */

int sl_sysfs_media_counters_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
#ifdef CONFIG_SYSFS
	struct sl_media_lgrp *media_lgrp;
	int                   rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media counters create");

	media_lgrp = sl_media_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);
	if (!media_lgrp) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media_lgrp_get failed");
		return -EFAULT;
	}

	rtn = kobject_init_and_add(&media_lgrp->counters_kobj, &media_counters, &media_lgrp->kobj,
				   "counters");
	if (rtn) {
		kobject_put(&media_lgrp->counters_kobj);
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			   "media counters create kobject_init_and_add failed [%d]", rtn);
		return rtn;
	}

	return 0;
#else /* CONFIG_SYSFS */
	return 0;
#endif /* CONFIG_SYSFS */
}

void sl_sysfs_media_counters_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
#ifdef CONFIG_SYSFS
	struct sl_media_lgrp *media_lgrp;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media counters delete");

	media_lgrp = sl_media_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);
	if (!media_lgrp) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media_lgrp_get failed");
		return;
	}

	kobject_put(&media_lgrp->counters_kobj);
#endif /* CONFIG_SYSFS */
}
