// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "linux/sl_media.h"
#include "sl_sysfs.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "data/sl_media_data_jack.h"
#include "base/sl_media_eeprom.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u8                    state;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	state = sl_media_jack_state_get(media_lgrp->media_jack);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"state show (media_lgrp = 0x%p, state = %u %s)", media_lgrp, state, sl_media_state_str(state));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_state_str(state));
}

static ssize_t vendor_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u32                   vendor;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	vendor = sl_media_lgrp_vendor_get(media_lgrp);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"vendor show (media_lgrp = 0x%p, vendor = %u %s)", media_lgrp, vendor, sl_media_vendor_str(vendor));
	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_vendor_str(vendor));
}

static ssize_t type_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u32                   type;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	type = sl_media_lgrp_type_get(media_lgrp);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"type show (media_lgrp = 0x%p, type = %u %s)", media_lgrp, type, sl_media_type_str(type));
	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_type_str(type));
}

static ssize_t length_cm_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u32                   length_cm;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	length_cm = sl_media_lgrp_length_get(media_lgrp);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "length_cm show (media_lgrp = 0x%p, length_cm = %u)",
		media_lgrp, length_cm);
	return scnprintf(buf, PAGE_SIZE, "%u\n", length_cm);
}

static ssize_t max_speed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u32                   max_speed;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	max_speed = sl_media_lgrp_max_speed_get(media_lgrp);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "max speed show (media_lgrp = 0x%p, max_speed = %u %s)",
		media_lgrp, max_speed, sl_media_speed_str(max_speed));
	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_speed_str(max_speed));
}

static ssize_t serial_num_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	char                  serial_num[SL_MEDIA_SERIAL_NUM_SIZE + 1];

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	sl_media_lgrp_serial_num_get(media_lgrp, serial_num);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "serial num show (media_lgrp = 0x%p, serial_num = %s)",
		media_lgrp, serial_num);
	return scnprintf(buf, PAGE_SIZE, "%s\n", serial_num);
}

static ssize_t hpe_part_num_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	char                  hpe_pn_str[SL_MEDIA_HPE_PN_SIZE + 1];

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	sl_media_lgrp_hpe_pn_get(media_lgrp, hpe_pn_str);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "serial num show (media_lgrp = 0x%p, hpe_pn = %s)",
		media_lgrp, hpe_pn_str);
	return scnprintf(buf, PAGE_SIZE, "%s\n", hpe_pn_str);
}

static ssize_t jack_num_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"jack_num show (media_lgrp = 0x%p, jack_num = %u)",
		media_lgrp, media_lgrp->media_jack->physical_num);

	return scnprintf(buf, PAGE_SIZE, "%u\n", media_lgrp->media_jack->physical_num);
}

static ssize_t jack_type_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u32                   density;
	u32                   jack_type;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	jack_type = sl_media_lgrp_jack_type_get(media_lgrp);
	density = sl_media_lgrp_jack_type_qsfp_density_get(media_lgrp);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "jack type show (media_lgrp = 0x%p, jack_type = %u %s)",
		media_lgrp, jack_type, sl_media_jack_type_str(jack_type, density));
	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_jack_type_str(jack_type, density));
}

static ssize_t furcation_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u32                   furcation;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	furcation = sl_media_lgrp_furcation_get(media_lgrp);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "furcation show (media_lgrp = 0x%p, furcation = %u %s)",
		media_lgrp, furcation, sl_media_furcation_str(furcation));
	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_furcation_str(furcation));
}

static ssize_t supported_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;
	bool                  not_supported;

	media_lgrp = container_of(kobj, struct sl_media_lgrp, kobj);
	ctl_lgrp = sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num);

	if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	not_supported = sl_media_lgrp_is_cable_not_supported(media_lgrp);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "supported show (media_lgrp = 0x%p, supported = %s)",
		media_lgrp, not_supported ? "no" : "yes");

	return scnprintf(buf, PAGE_SIZE, "%s\n", not_supported ? "no" : "yes");
}

static struct kobj_attribute media_state            = __ATTR_RO(state);
static struct kobj_attribute media_vendor           = __ATTR_RO(vendor);
static struct kobj_attribute media_type             = __ATTR_RO(type);
static struct kobj_attribute media_length_cm        = __ATTR_RO(length_cm);
static struct kobj_attribute media_max_speed        = __ATTR_RO(max_speed);
static struct kobj_attribute media_serial_num       = __ATTR_RO(serial_num);
static struct kobj_attribute media_hpe_part_num     = __ATTR_RO(hpe_part_num);
static struct kobj_attribute media_jack_num         = __ATTR_RO(jack_num);
static struct kobj_attribute media_jack_type        = __ATTR_RO(jack_type);
static struct kobj_attribute media_furcation        = __ATTR_RO(furcation);
static struct kobj_attribute media_supported        = __ATTR_RO(supported);

static struct attribute *media_attrs[] = {
	&media_state.attr,
	&media_vendor.attr,
	&media_type.attr,
	&media_length_cm.attr,
	&media_max_speed.attr,
	&media_serial_num.attr,
	&media_hpe_part_num.attr,
	&media_jack_num.attr,
	&media_jack_type.attr,
	&media_furcation.attr,
	&media_supported.attr,
	NULL
};
ATTRIBUTE_GROUPS(media);

static struct kobj_type media_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = media_groups,
};

static ssize_t host_interface_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp_speed_kobject *speed_kobj;
	struct sl_ctl_lgrp                 *ctl_lgrp;
	u32                                 speed;
	u32                                 type;

	speed_kobj = container_of(kobj, struct sl_media_lgrp_speed_kobject, kobj);

	ctl_lgrp = sl_ctl_lgrp_get(speed_kobj->media_lgrp->media_ldev->num, speed_kobj->media_lgrp->num);

	if (!sl_media_jack_is_cable_online(speed_kobj->media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(speed_kobj->media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	type = sl_media_lgrp_type_get(speed_kobj->media_lgrp);

	speed = speed_kobj->speed;

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"host interface show (media_lgrp = 0x%p, host_interface = %s)",
		speed_kobj->media_lgrp, sl_media_host_interface_str(speed, type));
	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_host_interface_str(speed, type));
}

static ssize_t projected_ber_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_media_lgrp_speed_kobject *speed_kobj;
	struct sl_ctl_lgrp                 *ctl_lgrp;
	u8                                  media_interface;
	u8                                  type;

	speed_kobj = container_of(kobj, struct sl_media_lgrp_speed_kobject, kobj);

	ctl_lgrp = sl_ctl_lgrp_get(speed_kobj->media_lgrp->media_ldev->num, speed_kobj->media_lgrp->num);

	if (!sl_media_jack_is_cable_online(speed_kobj->media_lgrp->media_jack)) {
		if (sl_media_jack_is_cable_format_invalid(speed_kobj->media_lgrp->media_jack))
			return scnprintf(buf, PAGE_SIZE, "can't read cable attributes\n");
		return scnprintf(buf, PAGE_SIZE, "no cable\n");
	}

	type = sl_media_lgrp_type_get(speed_kobj->media_lgrp);
	if (type == SL_MEDIA_TYPE_PEC || type == SL_MEDIA_TYPE_BKP) {
		sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
			"projected_ber show (media_lgrp = 0x%p, projected_ber = 0)", speed_kobj->media_lgrp);
		return scnprintf(buf, PAGE_SIZE, "0\n");
	}

	media_interface = sl_media_eeprom_media_interface_get(speed_kobj->media_lgrp->media_jack);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"projected_ber show (media_lgrp = 0x%p, projected_ber = %s)",
		speed_kobj->media_lgrp, sl_media_ber_str(media_interface));
	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_ber_str(media_interface));
}

static struct kobj_attribute media_speed_host_interface = __ATTR_RO(host_interface);
static struct kobj_attribute media_speed_projected_ber  = __ATTR_RO(projected_ber);

static struct attribute *media_speed_attrs[] = {
	&media_speed_host_interface.attr,
	&media_speed_projected_ber.attr,
	NULL
};
ATTRIBUTE_GROUPS(media_speed);

static struct kobj_type media_speed_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = media_speed_groups,
};

static void parent_speed_release(struct kobject *kobj)
{
}

static struct kobj_type parent_speed_info = {
	.release = parent_speed_release,
};

int sl_sysfs_media_create(struct sl_ctl_lgrp *ctl_lgrp)
{
	int                   rtn;
	u8                    state;
	struct sl_media_lgrp *media_lgrp;

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media create");

	if (!ctl_lgrp->parent_kobj) {
		sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media create missing parent");
		return 0;
	}

	media_lgrp = sl_media_lgrp_get(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num);
	if (!media_lgrp) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media_lgrp_get failed");
		return -EFAULT;
	}

	rtn = kobject_init_and_add(&media_lgrp->kobj, &media_info, ctl_lgrp->parent_kobj, "media");
	if (rtn) {
		kobject_put(&media_lgrp->kobj);
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "kobject_init_and_add failed [%d]", rtn);
		return rtn;
	}

	rtn = kobject_init_and_add(&media_lgrp->parent_speed_kobj, &parent_speed_info, &media_lgrp->kobj, "speeds");
	if (rtn) {
		kobject_put(&media_lgrp->parent_speed_kobj);
		kobject_put(&media_lgrp->kobj);
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "kobject_init_and_add failed [%d]", rtn);
		return rtn;
	}

	/*
	 * This if condition will be false for cassini here
	 */
	state = sl_media_jack_state_get(media_lgrp->media_jack);
	if (state == SL_MEDIA_JACK_CABLE_ONLINE) {
		/*
		 * kobj cleanup in the callee function
		 */
		rtn = sl_sysfs_media_speeds_create(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num);
		if (rtn) {
			sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media speeds create failed [%d]", rtn);
			return rtn;
		}
	}

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "create (media_kobj = 0x%p)", &media_lgrp->kobj);

	return 0;
}

int sl_sysfs_media_speeds_create(u8 ldev_num, u8 lgrp_num)
{
	int                   rtn;
	int                   i;
	struct sl_media_lgrp *media_lgrp;
	struct sl_ctl_lgrp   *ctl_lgrp;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);
	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);

	media_lgrp->supported_speeds_num = 0;
	if (media_lgrp->cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		for_each_set_bit(i, (unsigned long *)&media_lgrp->cable_info->media_attr.speeds_map, 32) {
			media_lgrp->speeds_kobj[media_lgrp->supported_speeds_num].media_lgrp = media_lgrp;
			media_lgrp->speeds_kobj[media_lgrp->supported_speeds_num].speed = BIT(i);
			rtn = kobject_init_and_add(&media_lgrp->speeds_kobj[media_lgrp->supported_speeds_num].kobj,
					&media_speed_info, &media_lgrp->parent_speed_kobj, sl_media_speed_str(BIT(i)));
			if (rtn) {
				for (i = 0; i <= media_lgrp->supported_speeds_num; ++i)
					kobject_put(&media_lgrp->speeds_kobj[i].kobj);
				kobject_put(&media_lgrp->parent_speed_kobj);
				kobject_put(&media_lgrp->kobj);
				sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "kobject_init_and_add failed [%d]", rtn);
				return rtn;
			}
			media_lgrp->supported_speeds_num++;
		}
	} else if (media_lgrp->cable_info->fake_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		for_each_set_bit(i, (unsigned long *)&media_lgrp->cable_info->stashed_media_attr.speeds_map, 32) {
			media_lgrp->speeds_kobj[media_lgrp->supported_speeds_num].media_lgrp = media_lgrp;
			media_lgrp->speeds_kobj[media_lgrp->supported_speeds_num].speed = BIT(i);
			rtn = kobject_init_and_add(&media_lgrp->speeds_kobj[media_lgrp->supported_speeds_num].kobj,
					&media_speed_info, &media_lgrp->parent_speed_kobj, sl_media_speed_str(BIT(i)));
			if (rtn) {
				for (i = 0; i <= media_lgrp->supported_speeds_num; ++i)
					kobject_put(&media_lgrp->speeds_kobj[i].kobj);
				kobject_put(&media_lgrp->parent_speed_kobj);
				kobject_put(&media_lgrp->kobj);
				sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "kobject_init_and_add failed [%d]", rtn);
				return rtn;
			}
			media_lgrp->supported_speeds_num++;
		}
	}

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media speed nodes created");

	return 0;
}

void sl_sysfs_media_delete(struct sl_ctl_lgrp *ctl_lgrp)
{
	struct sl_media_lgrp *media_lgrp;
	u8                    i;

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media delete");

	if (!ctl_lgrp->parent_kobj)
		return;

	media_lgrp = sl_media_lgrp_get(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num);
	if (!media_lgrp) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media_lgrp_get failed");
		return;
	}

	for (i = 0; i < media_lgrp->supported_speeds_num; ++i)
		kobject_put(&media_lgrp->speeds_kobj[i].kobj);

	kobject_put(&media_lgrp->parent_speed_kobj);
	kobject_put(&media_lgrp->kobj);
}