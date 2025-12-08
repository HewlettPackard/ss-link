// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/atomic.h>

#include <linux/hpe/sl/sl_media.h>

#include "base/sl_media_log.h"
#include "base/sl_media_eeprom.h"
#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_ldev.h"
#include "sl_asic.h"
#include "sl_ctrl_lgrp_notif.h"
#include "data/sl_media_data_lgrp.h"
#include "sl_ctrl_lgrp.h"
#include "sl_core_link.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_link_priv.h"

static struct sl_media_jack *media_jacks[SL_ASIC_MAX_LDEVS][SL_MEDIA_MAX_JACK_NUM];
static DEFINE_SPINLOCK(media_jacks_lock);

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

void sl_media_data_jack_del(u8 ldev_num, u8 jack_num)
{
	struct sl_media_jack *media_jack;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);
	if (!media_jack) {
		sl_media_log_dbg(NULL, LOG_NAME, "not found (jack_num = %u)", jack_num);
		return;
	}

	spin_lock(&media_jacks_lock);
	media_jacks[ldev_num][jack_num] = NULL;
	spin_unlock(&media_jacks_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "del (jack = 0x%p)", media_jack);

	kfree(media_jack);
}

int sl_media_data_jack_new(struct sl_media_ldev *media_ldev, u8 jack_num)
{
	struct sl_media_jack *media_jack;

	media_jack = sl_media_data_jack_get(media_ldev->num, jack_num);
	if (media_jack) {
		sl_media_log_err(NULL, LOG_NAME, "jack already exists");
		return -EEXIST;
	}

	media_jack = kzalloc(sizeof(struct sl_media_jack), GFP_KERNEL);
	if (!media_jack)
		return -ENOMEM;

	media_jack->magic             = SL_MEDIA_JACK_MAGIC;
	media_jack->num               = jack_num;
	media_jack->physical_num      = 1;
	media_jack->cable_db_idx      = -1;
	media_jack->fault_cause       = SL_MEDIA_FAULT_CAUSE_NONE;

	spin_lock_init(&(media_jack->data_lock));
	spin_lock_init(&(media_jack->log_lock));

	media_jack->media_ldev = media_ldev;

	media_jack->state = SL_MEDIA_JACK_CABLE_REMOVED;

	media_jack->temperature_value     = -1;
	media_jack->temperature_threshold = -1;

	sl_media_log_dbg(media_jack, LOG_NAME, "new (jack = 0x%p)", media_jack);

	spin_lock(&media_jacks_lock);
	media_jacks[media_ldev->num][jack_num] = media_jack;
	spin_unlock(&media_jacks_lock);

	return 0;
}

struct sl_media_jack *sl_media_data_jack_get(u8 ldev_num, u8 jack_num)
{
	struct sl_media_jack *media_jack;

	spin_lock(&media_jacks_lock);
	media_jack = media_jacks[ldev_num][jack_num];
	spin_unlock(&media_jacks_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "get (jack = 0x%p)", media_jack);

	return media_jack;
}

void sl_media_data_cable_serdes_settings_clr(struct sl_media_jack *media_jack)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "serdes settings clr");

	memset(&(media_jack->serdes_settings), 0, sizeof(struct sl_media_serdes_settings));
}

void sl_media_data_jack_eeprom_clr(struct sl_media_jack *media_jack)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "eeprom clr");

	memset(media_jack->eeprom_page0, 0, SL_MEDIA_EEPROM_PAGE_SIZE);
	memset(media_jack->eeprom_page1, 0, SL_MEDIA_EEPROM_PAGE_SIZE);
}

int sl_media_data_jack_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *media_attr)
{
	struct sl_media_lgrp *media_lgrp;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "media attr set (vendor = %d %s, type = 0x%X %s, length_cm = %d, speeds_map = 0x%X)",
			 media_attr->vendor, sl_media_vendor_str(media_attr->vendor),
			 media_attr->type, sl_media_type_str(media_attr->type),
			 media_attr->length_cm, media_attr->speeds_map);

	media_lgrp = sl_media_data_lgrp_get(cable_info->ldev_num, cable_info->lgrp_num);
	spin_lock(&media_jack->data_lock);
	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		spin_unlock(&media_jack->data_lock);
		sl_media_log_err_trace(media_jack, LOG_NAME, "media attr already set");
		return -EEXIST;
	}

	if (cable_info->fake_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		cable_info->stashed_media_attr = *media_attr;
		cable_info->real_cable_status = CABLE_MEDIA_ATTR_STASHED;
		spin_unlock(&media_jack->data_lock);
	} else {
		cable_info->media_attr = *media_attr;
		cable_info->real_cable_status = CABLE_MEDIA_ATTR_ADDED;
		spin_unlock(&media_jack->data_lock);
		if (media_lgrp)
			sl_media_data_jack_cable_if_present_send(media_lgrp);
	}

	return 0;
}

void sl_media_data_jack_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info)
{
	struct sl_media_lgrp *media_lgrp;

	sl_media_log_dbg(media_jack, LOG_NAME, "media attr clr");

	media_lgrp = sl_media_data_lgrp_get(cable_info->ldev_num, cable_info->lgrp_num);
	spin_lock(&media_jack->data_lock);
	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_REMOVED) {
		spin_unlock(&media_jack->data_lock);
		sl_media_log_dbg(media_jack, LOG_NAME, "no media attr to clear");
		return;
	}

	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED)
		memset(&(cable_info->media_attr), 0, sizeof(struct sl_media_attr));

	cable_info->real_cable_status = CABLE_MEDIA_ATTR_REMOVED;
	spin_unlock(&media_jack->data_lock);

	if (media_lgrp) {
		/* Only send notification if both real and fake cables are removed */
		if (cable_info->fake_cable_status == CABLE_MEDIA_ATTR_REMOVED)
			sl_media_data_jack_cable_if_not_present_send(media_lgrp);
	}
}

void sl_media_data_jack_cable_if_present_send(struct sl_media_lgrp *media_lgrp)
{
	int                      rtn;
	union sl_lgrp_notif_info info;

	sl_media_log_dbg(media_lgrp, LOG_NAME, "present send");

	spin_lock(&media_lgrp->media_jack->data_lock);
	if ((media_lgrp->cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED) ||
		(media_lgrp->cable_info->fake_cable_status == CABLE_MEDIA_ATTR_ADDED)) {
		info.media_attr = media_lgrp->cable_info->media_attr;
		rtn = sl_ctrl_lgrp_notif_enqueue(sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num),
			SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_PRESENT, &info, 0);
		if (rtn)
			sl_media_log_warn_trace(media_lgrp, LOG_NAME,
				"present_send ctrl_lgrp_notif_enqueue failed [%d]", rtn);
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);
}

void sl_media_data_jack_cable_if_not_present_send(struct sl_media_lgrp *media_lgrp)
{
	int rtn;

	sl_media_log_dbg(media_lgrp, LOG_NAME, "not present send");

	spin_lock(&media_lgrp->media_jack->data_lock);
	if (media_lgrp->cable_info->real_cable_status == CABLE_MEDIA_ATTR_REMOVED) {
		rtn = sl_ctrl_lgrp_notif_enqueue(sl_ctrl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num),
			SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_NOT_PRESENT, NULL, 0);
		if (rtn)
			sl_media_log_warn_trace(media_lgrp, LOG_NAME,
				"not_present_send ctrl_lgrp_notif_enqueue failed [%d]", rtn);
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);
}

bool sl_media_data_jack_cable_is_high_temp(struct sl_media_jack *media_jack)
{
	bool is_high_temp;

	spin_lock(&media_jack->data_lock);
	is_high_temp = media_jack->is_high_temp;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME,
		"data jack cable is high temp (%s)",
		(media_jack->is_high_temp) ? "yes" : "no");

	return is_high_temp;
}

int sl_media_data_jack_high_temp_link_down(struct sl_media_jack *media_jack)
{
	int                  rtn;
	int                  i;
	struct sl_ctrl_link *ctrl_link;
	u8                   ldev_num;
	u8                   lgrp_num;
	u8                   link_num;

	sl_media_log_dbg(media_jack, LOG_NAME, "high temp link down");

	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_HIGH_TEMP);

	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;

		for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
			ctrl_link = sl_ctrl_link_get(ldev_num, lgrp_num, link_num);
			if (!ctrl_link)
				continue;

			rtn = sl_ctrl_link_async_down(ctrl_link, SL_LINK_DOWN_CAUSE_HIGH_TEMP_FAULT_MAP);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME,
						       "high temp link down async_down failed [%d]", rtn);
		}
	}
	return 0;
}

void sl_media_data_jack_headshell_busy_set(struct sl_media_jack *media_jack, int value)
{
	atomic_set(&media_jack->is_headshell_busy, value);
}

bool sl_media_data_jack_is_headshell_busy(struct sl_media_jack *media_jack)
{
	return (atomic_read(&media_jack->is_headshell_busy) == SL_MEDIA_JACK_HEADSHELL_BUSY);
}
