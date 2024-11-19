// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/types.h>

#include "base/sl_media_log.h"

#include "sl_asic.h"

#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_jack.h"

#define LOG_NAME SL_MEDIA_LGRP_LOG_NAME

int sl_media_lgrp_new(u8 ldev_num, u8 lgrp_num)
{
	return sl_media_data_lgrp_new(ldev_num, lgrp_num);
}

void sl_media_lgrp_del(u8 ldev_num, u8 lgrp_num)
{
	sl_media_data_lgrp_del(ldev_num, lgrp_num);
}

struct sl_media_lgrp *sl_media_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	return sl_media_data_lgrp_get(ldev_num, lgrp_num);
}

void sl_media_lgrp_media_attr_get(u8 ldev_num, u8 lgrp_num, struct sl_media_attr *media_attr)
{
	struct sl_media_lgrp *media_lgrp;
	unsigned long         irq_flags;

	media_lgrp = sl_media_data_lgrp_get(ldev_num, lgrp_num);
	if (!media_lgrp) {
		sl_media_log_err(NULL, SL_MEDIA_LGRP_LOG_NAME, "media_data_lgrp_get failed");
		return;
	}

	sl_media_log_dbg(media_lgrp, SL_MEDIA_LGRP_LOG_NAME, "media_attr_get");

	spin_lock_irqsave(&media_lgrp->media_jack->data_lock, irq_flags);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		*media_attr = media_lgrp->cable_info->media_attr;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		*media_attr = media_lgrp->cable_info->stashed_media_attr;
		break;
	default:
		memset(media_attr, 0, sizeof(struct sl_media_attr));
	}
	spin_unlock_irqrestore(&media_lgrp->media_jack->data_lock, irq_flags);
}

void sl_media_lgrp_media_serdes_settings_get(u8 ldev_num, u8 lgrp_num,
	struct sl_media_serdes_settings *media_serdes_settings)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_data_lgrp_get(ldev_num, lgrp_num);
	sl_media_log_dbg(media_lgrp, SL_MEDIA_LGRP_LOG_NAME, "media serdes settings get");

	*media_serdes_settings = media_lgrp->media_jack->serdes_settings;
}

bool sl_media_lgrp_cable_type_is_active(u8 ldev_num, u8 lgrp_num)
{
	struct sl_media_attr media_attr;

	sl_media_lgrp_media_attr_get(ldev_num, lgrp_num, &media_attr);

	return (media_attr.type == SL_MEDIA_TYPE_AOC ||
			media_attr.type == SL_MEDIA_TYPE_AEC);
}

void sl_media_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id)
{
	sl_media_data_lgrp_connect_id_set(sl_media_data_lgrp_get(ldev_num, lgrp_num), connect_id);
}

void sl_media_lgrp_real_cable_if_present_send(u8 ldev_num, u8 lgrp_num)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_data_lgrp_get(ldev_num, lgrp_num);
	sl_media_log_dbg(media_lgrp, SL_MEDIA_LGRP_LOG_NAME, "media lgrp real cable if present send");
	if (media_lgrp->cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED)
		sl_media_data_jack_cable_present_send(media_lgrp);
}

u32 sl_media_lgrp_vendor_get(struct sl_media_lgrp *media_lgrp)
{
	u32 vendor;

	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		vendor = media_lgrp->cable_info->media_attr.vendor;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		vendor = media_lgrp->cable_info->stashed_media_attr.vendor;
		break;
	default:
		vendor = SL_MEDIA_VENDOR_INVALID;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return vendor;
}

u32 sl_media_lgrp_type_get(struct sl_media_lgrp *media_lgrp)
{
	u32 type;

	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		type = media_lgrp->cable_info->media_attr.type;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		type = media_lgrp->cable_info->stashed_media_attr.type;
		break;
	default:
		type = SL_MEDIA_TYPE_INVALID;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return type;
}

u32 sl_media_lgrp_length_get(struct sl_media_lgrp *media_lgrp)
{
	u32 length_cm;

	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		length_cm = media_lgrp->cable_info->media_attr.length_cm;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		length_cm = media_lgrp->cable_info->stashed_media_attr.length_cm;
		break;
	default:
		length_cm = 0;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return length_cm;
}

u32 sl_media_lgrp_max_speed_get(struct sl_media_lgrp *media_lgrp)
{
	u32 max_speed;

	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		max_speed = media_lgrp->cable_info->media_attr.max_speed;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		max_speed = media_lgrp->cable_info->stashed_media_attr.max_speed;
		break;
	default:
		max_speed = 0;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return max_speed;
}

void sl_media_lgrp_serial_num_get(struct sl_media_lgrp *media_lgrp, char *serial_num)
{
	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		strncpy(serial_num, media_lgrp->cable_info->media_attr.serial_num, SL_MEDIA_SERIAL_NUM_SIZE + 1);
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		strncpy(serial_num, media_lgrp->cable_info->stashed_media_attr.serial_num,
			SL_MEDIA_SERIAL_NUM_SIZE + 1);
		break;
	default:
		memset(serial_num, '0', SL_MEDIA_SERIAL_NUM_SIZE);
		serial_num[SL_MEDIA_SERIAL_NUM_SIZE] = '\0';
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);
}

void sl_media_lgrp_hpe_pn_get(struct sl_media_lgrp *media_lgrp, char *hpe_pn_str)
{
	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		strncpy(hpe_pn_str, media_lgrp->cable_info->media_attr.hpe_pn_str, SL_MEDIA_HPE_PN_SIZE + 1);
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		strncpy(hpe_pn_str, media_lgrp->cable_info->stashed_media_attr.hpe_pn_str, SL_MEDIA_HPE_PN_SIZE + 1);
		break;
	default:
		memset(hpe_pn_str, '0', SL_MEDIA_HPE_PN_SIZE);
		hpe_pn_str[SL_MEDIA_HPE_PN_SIZE] = '\0';
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);
}

u32 sl_media_lgrp_jack_type_get(struct sl_media_lgrp *media_lgrp)
{
	u32 jack_type;

	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		jack_type = media_lgrp->cable_info->media_attr.jack_type;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		jack_type = media_lgrp->cable_info->stashed_media_attr.jack_type;
		break;
	default:
		jack_type = SL_MEDIA_JACK_TYPE_INVALID;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return jack_type;
}

u32 sl_media_lgrp_jack_type_qsfp_density_get(struct sl_media_lgrp *media_lgrp)
{
	u32 density;

	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		density = media_lgrp->cable_info->media_attr.jack_type_info.qsfp.density;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		density = media_lgrp->cable_info->stashed_media_attr.jack_type_info.qsfp.density;
		break;
	default:
		density = SL_MEDIA_QSFP_DENSITY_INVALID;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return density;
}

u32 sl_media_lgrp_furcation_get(struct sl_media_lgrp *media_lgrp)
{
	u32 furcation;

	spin_lock(&media_lgrp->media_jack->data_lock);
	switch (media_lgrp->cable_info->real_cable_status) {
	case CABLE_MEDIA_ATTR_ADDED:
		furcation = media_lgrp->cable_info->media_attr.furcation;
		break;
	case CABLE_MEDIA_ATTR_STASHED:
		furcation = media_lgrp->cable_info->stashed_media_attr.furcation;
		break;
	default:
		furcation = SL_MEDIA_FURCATION_INVALID;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return furcation;
}

bool sl_media_lgrp_is_cable_not_supported(struct sl_media_lgrp *media_lgrp)
{
	bool not_supported;

	spin_lock(&media_lgrp->media_jack->data_lock);
	not_supported = media_lgrp->media_jack->is_cable_not_supported;
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return not_supported;
}
