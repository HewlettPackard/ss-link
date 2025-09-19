// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_asic.h"
#include "sl_media_jack.h"
#include "data/sl_media_data_cable_db.h"
#include "data/sl_media_data_cable_db_ops.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_CABLE_LOG_NAME

#define SL_MEDIA_TYPE_SERDES 1

static u32 sl_media_data_cable_length_round_up(u32 length_cm)
{
	u32 new_len;

	new_len = (length_cm / 10) * 10;

	if ((length_cm % 10) != 0)
		length_cm = new_len + 10;

	return length_cm;
}

int sl_media_data_cable_db_ops_cable_validate(struct sl_media_attr *media_attr, struct sl_media_jack *media_jack)
{
	int indexer;

	sl_media_log_dbg(NULL, LOG_NAME, "cable validate");

	/*
	 * Check for loopback module
	 */
	if (media_attr->vendor == SL_MEDIA_VENDOR_MULTILANE) {
		media_attr->hpe_pn       = 1;
		media_attr->type         = SL_MEDIA_TYPE_PEC;
		media_attr->length_cm    = 100;
		media_attr->furcation    = SL_MEDIA_FURCATION_X1;
		media_attr->max_speed    = SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		media_attr->speeds_map   = SL_MEDIA_SPEEDS_SUPPORT_CK_400G |
					   SL_MEDIA_SPEEDS_SUPPORT_BS_200G |
					   SL_MEDIA_SPEEDS_SUPPORT_BJ_100G |
					   SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		media_jack->cable_db_idx = -1;
		return 0;
	}

	/*
	 * Linear search through the cable_db array
	 */
	for (indexer = 0; indexer < ARRAY_SIZE(cable_db); ++indexer) {
		if (cable_db[indexer].hpe_pn == media_attr->hpe_pn &&
		    cable_db[indexer].vendor == media_attr->vendor &&
		    cable_db[indexer].type == media_attr->type &&
		    sl_media_data_cable_length_round_up(cable_db[indexer].length_cm) == media_attr->length_cm) {
			media_attr->shape = cable_db[indexer].shape;
			media_attr->max_speed = cable_db[indexer].max_speed;
			media_jack->is_supported_ss200_cable = cable_db[indexer].is_supported_ss200_cable;
			media_jack->cable_db_idx = indexer;
			return 0;
		}
	}

	return -ENOENT;
}

int sl_media_data_cable_db_ops_serdes_settings_get(struct sl_media_jack *media_jack, u32 media_type, u32 flags)
{
	sl_media_log_dbg(media_jack, LOG_NAME,
		"serdes settings get (media_type = 0x%X %s, flags = 0x%X)",
		media_type, sl_media_type_str(media_type), flags);

	if (flags & SL_MEDIA_TYPE_NOT_SUPPORTED) {
		if (media_type == SL_MEDIA_TYPE_AEC || media_type == SL_MEDIA_TYPE_POC) {
			media_jack->serdes_settings.pre1   = -4;
			media_jack->serdes_settings.pre2   = 0;
			media_jack->serdes_settings.pre3   = 0;
			media_jack->serdes_settings.cursor = 98;
			media_jack->serdes_settings.post1  = 0;
			media_jack->serdes_settings.post2  = 0;
		} else if (media_type == SL_MEDIA_TYPE_AOC) {
			media_jack->serdes_settings.pre1   = -12;
			media_jack->serdes_settings.pre2   = 0;
			media_jack->serdes_settings.pre3   = 0;
			media_jack->serdes_settings.cursor = 98;
			media_jack->serdes_settings.post1  = -4;
			media_jack->serdes_settings.post2  = 0;
		} else {
			media_jack->serdes_settings.pre1   = -20;
			media_jack->serdes_settings.pre2   = 0;
			media_jack->serdes_settings.pre3   = 0;
			media_jack->serdes_settings.cursor = 116;
			media_jack->serdes_settings.post1  = 0;
			media_jack->serdes_settings.post2  = 0;
		}
	} else if (flags & SL_MEDIA_TYPE_BACKPLANE) {
		media_jack->serdes_settings.pre1   = 0;
		media_jack->serdes_settings.pre2   = 0;
		media_jack->serdes_settings.pre3   = 0;
		media_jack->serdes_settings.cursor = 100;
		media_jack->serdes_settings.post1  = 0;
		media_jack->serdes_settings.post2  = 0;
	} else if (flags & SL_MEDIA_TYPE_LOOPBACK) {
		media_jack->serdes_settings.pre1   = 0;
		media_jack->serdes_settings.pre2   = 0;
		media_jack->serdes_settings.pre3   = 0;
		media_jack->serdes_settings.cursor = 100;
		media_jack->serdes_settings.post1  = 0;
		media_jack->serdes_settings.post2  = 0;
	} else {
		if (media_jack->cable_db_idx < 0 || media_jack->cable_db_idx >= ARRAY_SIZE(cable_db)) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "serdes settings get - invalid idx (%d)",
					media_jack->cable_db_idx);
			return -ENOENT;
		}
		media_jack->serdes_settings = cable_db[media_jack->cable_db_idx].serdes_settings;
	}
	media_jack->serdes_settings.media = SL_MEDIA_TYPE_SERDES;

	return 0;
}
