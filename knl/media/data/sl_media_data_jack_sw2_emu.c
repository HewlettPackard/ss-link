// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>

#include <linux/sl_media.h>

#include "sl_asic.h"
#include "base/sl_media_log.h"
#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_jack_sw2_emu.h"
#include "sl_media_data_lgrp.h"

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

int sl_media_data_jack_fake_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *fake_media_attr)
{
	struct sl_media_lgrp *media_lgrp;

	sl_media_log_dbg(media_jack, LOG_NAME,
		"fake media attr set (vendor = %d %s, type = 0x%X %s, len = %d, speeds_map = 0x%X)",
		fake_media_attr->vendor, sl_media_vendor_str(fake_media_attr->vendor),
		fake_media_attr->type, sl_media_type_str(fake_media_attr->type),
		fake_media_attr->length_cm, fake_media_attr->speeds_map);

	media_lgrp = sl_media_data_lgrp_get(cable_info->ldev_num, cable_info->lgrp_num);
	spin_lock(&media_jack->data_lock);
	if (cable_info->fake_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		spin_unlock(&media_jack->data_lock);
		sl_media_log_err(media_jack, LOG_NAME, "fake media attr already set");
		return -EEXIST;
	}

	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		cable_info->stashed_media_attr = cable_info->media_attr;
		cable_info->real_cable_status = CABLE_MEDIA_ATTR_STASHED;
	}
	cable_info->media_attr = *fake_media_attr;
	cable_info->fake_cable_status = CABLE_MEDIA_ATTR_ADDED;
	spin_unlock(&media_jack->data_lock);

	if (media_lgrp) {
		if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_STASHED)
			sl_media_data_jack_cable_if_not_present_send(media_lgrp);

		sl_media_data_jack_cable_if_present_send(media_lgrp);
	}

	return 0;
}

void sl_media_data_jack_fake_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info)
{
	struct sl_media_lgrp *media_lgrp;

	sl_media_log_dbg(media_jack, LOG_NAME, "fake media attr clr");

	media_lgrp = sl_media_data_lgrp_get(cable_info->ldev_num, cable_info->lgrp_num);
	spin_lock(&media_jack->data_lock);
	if (cable_info->fake_cable_status == CABLE_MEDIA_ATTR_REMOVED) {
		spin_unlock(&media_jack->data_lock);
		sl_media_log_dbg(media_jack, LOG_NAME, "no fake media attr to clear");
		return;
	}

	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_STASHED) {
		cable_info->media_attr = cable_info->stashed_media_attr;
		cable_info->real_cable_status = CABLE_MEDIA_ATTR_ADDED;
	} else {
		memset(&(cable_info->media_attr), 0, sizeof(struct sl_media_attr));
	}
	cable_info->fake_cable_status = CABLE_MEDIA_ATTR_REMOVED;
	spin_unlock(&media_jack->data_lock);

	if (media_lgrp) {
		sl_media_data_jack_cable_if_not_present_send(media_lgrp);
		if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED)
			sl_media_data_jack_cable_if_present_send(media_lgrp);
	}
}
