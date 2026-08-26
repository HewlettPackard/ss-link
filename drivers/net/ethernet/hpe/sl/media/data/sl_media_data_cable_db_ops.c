// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024-2026 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_asic.h"
#include "sl_media_jack.h"
#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"
#include "data/sl_media_cable_db_load.h"
#include "data/sl_media_data_cable_db_ops.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_CABLE_LOG_NAME

#define SL_MEDIA_TYPE_SERDES 1

int sl_media_data_cable_db_ops_cable_validate(struct sl_media_attr *media_attr, struct sl_media_jack *media_jack)
{
	struct sl_media_cable_attr entry;
	struct sl_media_ldev      *media_ldev;
	u32                        indexer;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "validate (hpe_part_num = %d %s, vendor = %d %s, type = 0x%X %s)",
			 media_attr->hpe_pn, media_attr->hpe_pn_str,
			 media_attr->vendor, sl_media_vendor_str(media_attr->vendor),
			 media_attr->type, sl_media_type_str(media_attr->type));

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
		media_jack->cable_db_idx = SL_MEDIA_DB_IDX_NONE;

		media_jack->is_cable_unsupported = false;
		return 0;
	}

	// FIXME: do we need this check?
	media_ldev = media_jack->media_ldev;
	if (!media_ldev || !media_ldev->cable_db.data) {
		sl_media_log_err(NULL, LOG_NAME, "cable validate cable_db not loaded");
		return -ENOENT;
	}

	for (indexer = 0; indexer < media_ldev->cable_db.count; ++indexer) {
		sl_media_data_cable_db_entry_get_by_idx(media_ldev, indexer, &entry);

		if (entry.hpe_pn != media_attr->hpe_pn)
			continue;
		if (entry.type != media_attr->type)
			continue;
		if (media_attr->vendor == SL_MEDIA_VENDOR_LEONI ||
		    media_attr->vendor == SL_MEDIA_VENDOR_BIZLINK)
			if (entry.vendor != SL_MEDIA_VENDOR_LEONI &&
			    entry.vendor != SL_MEDIA_VENDOR_BIZLINK)
				continue;

		media_attr->shape                    = entry.shape;
		media_attr->max_speed                = entry.max_speed;
		media_jack->is_supported_ss200_cable = entry.is_supported_ss200_cable;
		media_jack->cable_db_idx             = indexer;
		return 0;
	}

	media_jack->is_cable_unsupported = true;
	sl_media_log_dbg(media_jack, LOG_NAME, "validate not found");

	return -ENOENT;
}

int sl_media_data_cable_db_ops_serdes_settings_get(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	struct sl_media_cable_attr entry;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "serdes settings get (media_type = 0x%X %s)",
			 media_attr->type, sl_media_type_str(media_attr->type));

	if (media_jack->is_cable_unsupported) {
		sl_media_log_warn_trace(media_jack, LOG_NAME, "serdes setting get unsuppported cable");
		if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr->type)) {
			media_jack->serdes_settings.pre1   = -12;
			media_jack->serdes_settings.pre2   = 0;
			media_jack->serdes_settings.pre3   = 0;
			media_jack->serdes_settings.cursor = 98;
			media_jack->serdes_settings.post1  = -4;
			media_jack->serdes_settings.post2  = 0;
		} else {
			media_jack->serdes_settings.pre1   = 0;
			media_jack->serdes_settings.pre2   = 0;
			media_jack->serdes_settings.pre3   = 0;
			media_jack->serdes_settings.cursor = 100;
			media_jack->serdes_settings.post1  = 0;
			media_jack->serdes_settings.post2  = 0;
		}
	} else if (media_attr->jack_type == SL_MEDIA_JACK_TYPE_BACKPLANE) {
		media_jack->serdes_settings.pre1   = 0;
		media_jack->serdes_settings.pre2   = 0;
		media_jack->serdes_settings.pre3   = 0;
		media_jack->serdes_settings.cursor = 100;
		media_jack->serdes_settings.post1  = 0;
		media_jack->serdes_settings.post2  = 0;
	} else if (media_attr->vendor == SL_MEDIA_VENDOR_MULTILANE) {
		media_jack->serdes_settings.pre1   = 0;
		media_jack->serdes_settings.pre2   = 0;
		media_jack->serdes_settings.pre3   = 0;
		media_jack->serdes_settings.cursor = 100;
		media_jack->serdes_settings.post1  = 0;
		media_jack->serdes_settings.post2  = 0;
	} else if (media_jack->cable_db_idx == SL_MEDIA_DB_IDX_NONE) {
		media_jack->serdes_settings.pre1   = 0;
		media_jack->serdes_settings.pre2   = 0;
		media_jack->serdes_settings.pre3   = 0;
		media_jack->serdes_settings.cursor = 100;
		media_jack->serdes_settings.post1  = 0;
		media_jack->serdes_settings.post2  = 0;
	} else {
		sl_media_data_cable_db_entry_get(media_jack, &entry);
		media_jack->serdes_settings = entry.serdes_settings;
	}
	media_jack->serdes_settings.media = SL_MEDIA_TYPE_SERDES;

	return 0;
}
