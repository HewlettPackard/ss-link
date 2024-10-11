// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>

#include "sl_asic.h"
#include "sl_media_jack.h"
#include "sl_media_data_cable_db.h"
#include "sl_media_data_cable_db_ops.h"
#include "uapi/sl_media.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_CABLE_LOG_NAME

#define SL_MEDIA_TYPE_SERDES 1

int sl_media_data_cable_db_ops_cable_validate(struct sl_media_attr *media_attr, struct sl_media_jack *media_jack)
{
	int target;
	int lower_idx;
	int upper_idx;
	int middle_idx;
	int req_idx;
	int indexer;

	sl_media_log_dbg(NULL, LOG_NAME, "cable validate");

	/*
	 * Check for loopback module
	 */
	if (media_attr->vendor == SL_MEDIA_VENDOR_MULTILANE) {
		media_attr->hpe_pn     = 1;
		media_attr->type       = SL_MEDIA_TYPE_PEC;
		media_attr->length_cm  = 100;
		media_attr->furcation  = SL_MEDIA_FURCATION_X1;
		media_attr->max_speed  = SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		media_attr->speeds_map = SL_MEDIA_SPEEDS_SUPPORT_CK_400G |
					 SL_MEDIA_SPEEDS_SUPPORT_BS_200G |
					 SL_MEDIA_SPEEDS_SUPPORT_BJ_100G |
					 SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		media_jack->cable_db_idx = -1;
		goto out_success;
	}

	target     = media_attr->hpe_pn;
	lower_idx  = 0;
	upper_idx  = ARRAY_SIZE(cable_db) - 1;
	middle_idx = 0;
	req_idx    = -1;
	indexer    = 0;

	while (lower_idx <= upper_idx) {
		middle_idx = ((unsigned int)lower_idx + (unsigned int)upper_idx) >> 1;
		if (cable_db[middle_idx].hpe_pn > target) {
			upper_idx = middle_idx - 1;
		} else if (cable_db[middle_idx].hpe_pn < target) {
			lower_idx = middle_idx + 1;
		} else {
			req_idx = middle_idx;
			break;
		}
	}

	if (req_idx == -1)
		goto out_failure;

	indexer = req_idx - 2;
	if (cable_db[req_idx].vendor == media_attr->vendor &&
		cable_db[req_idx].hpe_pn == media_attr->hpe_pn &&
		cable_db[req_idx].type == media_attr->type &&
		cable_db[req_idx].length_cm == media_attr->length_cm) {
		media_attr->max_speed = cable_db[req_idx].max_speed;
		media_jack->cable_db_idx = req_idx;
		goto out_success;
	} else {
		while (indexer != req_idx + 3) {
			if (indexer == req_idx) {
				indexer++;
				continue;
			}
			if (cable_db[indexer].vendor == media_attr->vendor &&
				cable_db[indexer].hpe_pn == media_attr->hpe_pn &&
				cable_db[indexer].type == media_attr->type &&
				cable_db[indexer].length_cm == media_attr->length_cm) {
				media_attr->max_speed = cable_db[indexer].max_speed;
				media_jack->cable_db_idx = indexer;
				goto out_success;
			}
			indexer++;
		}
	}

out_failure:
	return -ENOENT;
out_success:
	return 0;
}

int sl_media_data_cable_db_ops_serdes_settings_get(struct sl_media_jack *media_jack, u32 flags)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "serdes settings get");

	if (flags & SL_MEDIA_TYPE_NOT_SUPPORTED) {
		media_jack->serdes_settings.pre1   = -20;
		media_jack->serdes_settings.pre2   = 0;
		media_jack->serdes_settings.pre3   = 0;
		media_jack->serdes_settings.cursor = 116;
		media_jack->serdes_settings.post1  = 0;
		media_jack->serdes_settings.post2  = 0;
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
			sl_media_log_err(media_jack, LOG_NAME, "serdes settings get - invalid idx (%d)",
					media_jack->cable_db_idx);
			return -ENOENT;
		}
		media_jack->serdes_settings = cable_db[media_jack->cable_db_idx].serdes_settings;
	}
	media_jack->serdes_settings.media = SL_MEDIA_TYPE_SERDES;

	return 0;
}
