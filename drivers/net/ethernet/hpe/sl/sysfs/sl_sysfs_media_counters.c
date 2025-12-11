// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_sysfs.h"
#include "sl_log.h"
#include "sl_media_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_media_counters.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t cause_eeprom_format_invalid_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_EEPROM_FORMAT_INVALID, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause eeprom format invalid show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_eeprom_vendor_invalid_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_EEPROM_VENDOR_INVALID, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause eeprom vendor invalid show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause eeprom jack io show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_online_status_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_ONLINE_STATUS_GET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause online status get show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_online_timedout_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_ONLINE_TIMEDOUT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause online timedout show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_online_jack_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_ONLINE_JACK_IO, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause online jack io show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_online_jack_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_ONLINE_JACK_GET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause online jack get show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause serdes settings get show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_scan_status_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SCAN_STATUS_GET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause scan status get show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_scan_hdl_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SCAN_HDL_GET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause scan hdl get show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_scan_jack_get_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_SCAN_JACK_GET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause scan jack get show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause media attr set show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_intr_event_jack_io_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_INTR_EVENT_JACK_IO, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause intr event jack io show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_power_set_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_POWER_SET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause power set show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift down jack io show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift down jack io low power set show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift down jack io high power set show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift up jack io show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift up jack io low power set show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift up jack io high power set show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
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
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause shift state jack io show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_offline_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_OFFLINE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause offline show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_high_temp_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u32                   counter;
	u32                   rtn;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, counters_kobj);

	rtn = sl_ctrl_media_cause_counter_get(media_lgrp->media_jack, MEDIA_CAUSE_HIGH_TEMP, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	ctrl_lgrp = sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media cause high temp show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static struct kobj_attribute media_cause_eeprom_format_invalid             =
			     __ATTR_RO(cause_eeprom_format_invalid);
static struct kobj_attribute media_cause_eeprom_vendor_invalid             =
			     __ATTR_RO(cause_eeprom_vendor_invalid);
static struct kobj_attribute media_cause_eeprom_jack_io                    = __ATTR_RO(cause_eeprom_jack_io);
static struct kobj_attribute media_cause_online_status_get                 = __ATTR_RO(cause_online_status_get);
static struct kobj_attribute media_cause_online_timedout                   = __ATTR_RO(cause_online_timedout);
static struct kobj_attribute media_cause_online_jack_io                    = __ATTR_RO(cause_online_jack_io);
static struct kobj_attribute media_cause_online_jack_get                   = __ATTR_RO(cause_online_jack_get);
static struct kobj_attribute media_cause_serdes_settings_get               = __ATTR_RO(cause_serdes_settings_get);
static struct kobj_attribute media_cause_scan_status_get                   = __ATTR_RO(cause_scan_status_get);
static struct kobj_attribute media_cause_scan_hdl_get                      = __ATTR_RO(cause_scan_hdl_get);
static struct kobj_attribute media_cause_scan_jack_get                     = __ATTR_RO(cause_scan_jack_get);
static struct kobj_attribute media_cause_media_attr_set                    = __ATTR_RO(cause_media_attr_set);
static struct kobj_attribute media_cause_intr_event_jack_io                = __ATTR_RO(cause_intr_event_jack_io);
static struct kobj_attribute media_cause_power_set                         = __ATTR_RO(cause_power_set);
static struct kobj_attribute media_cause_shift_down_jack_io                = __ATTR_RO(cause_shift_down_jack_io);
static struct kobj_attribute media_cause_shift_down_jack_io_low_power_set  =
			     __ATTR_RO(cause_shift_down_jack_io_low_power_set);
static struct kobj_attribute media_cause_shift_down_jack_io_high_power_set =
			     __ATTR_RO(cause_shift_down_jack_io_high_power_set);
static struct kobj_attribute media_cause_shift_up_jack_io                  = __ATTR_RO(cause_shift_up_jack_io);
static struct kobj_attribute media_cause_shift_up_jack_io_low_power_set    =
			     __ATTR_RO(cause_shift_up_jack_io_low_power_set);
static struct kobj_attribute media_cause_shift_up_jack_io_high_power_set   =
			     __ATTR_RO(cause_shift_up_jack_io_high_power_set);
static struct kobj_attribute media_cause_shift_state_jack_io               =
			     __ATTR_RO(cause_shift_state_jack_io);
static struct kobj_attribute media_cause_offline                           = __ATTR_RO(cause_offline);
static struct kobj_attribute media_cause_high_temp                         = __ATTR_RO(cause_high_temp);

static struct attribute *media_counters_attrs[] = {
	&media_cause_eeprom_format_invalid.attr,
	&media_cause_eeprom_vendor_invalid.attr,
	&media_cause_eeprom_jack_io.attr,
	&media_cause_online_status_get.attr,
	&media_cause_online_timedout.attr,
	&media_cause_online_jack_io.attr,
	&media_cause_online_jack_get.attr,
	&media_cause_serdes_settings_get.attr,
	&media_cause_scan_status_get.attr,
	&media_cause_scan_hdl_get.attr,
	&media_cause_scan_jack_get.attr,
	&media_cause_media_attr_set.attr,
	&media_cause_intr_event_jack_io.attr,
	&media_cause_power_set.attr,
	&media_cause_shift_down_jack_io.attr,
	&media_cause_shift_down_jack_io_low_power_set.attr,
	&media_cause_shift_down_jack_io_high_power_set.attr,
	&media_cause_shift_up_jack_io.attr,
	&media_cause_shift_up_jack_io_low_power_set.attr,
	&media_cause_shift_up_jack_io_high_power_set.attr,
	&media_cause_shift_state_jack_io.attr,
	&media_cause_offline.attr,
	&media_cause_high_temp.attr,
	NULL
};
ATTRIBUTE_GROUPS(media_counters);

static struct kobj_type media_counters = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = media_counters_groups,
};

int sl_sysfs_media_counters_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
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
}

void sl_sysfs_media_counters_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	struct sl_media_lgrp *media_lgrp;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media counters delete");

	media_lgrp = sl_media_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);
	if (!media_lgrp) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media_lgrp_get failed");
		return;
	}

	kobject_put(&media_lgrp->counters_kobj);
}
