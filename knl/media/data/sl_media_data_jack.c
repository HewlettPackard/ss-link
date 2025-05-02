// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sl_media.h>

#include "base/sl_media_log.h"
#include "base/sl_media_eeprom.h"

#include "uapi/sl_media.h"
#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_ldev.h"
#include "sl_asic.h"

#include "sl_ctl_lgrp_notif.h"
#include "sl_media_data_lgrp.h"
#include "sl_media_data_jack_rosetta.h"

#include "sl_ctl_lgrp.h"

static struct sl_media_jack *media_jacks[SL_ASIC_MAX_LDEVS][SL_MEDIA_MAX_JACK_NUM];
static DEFINE_SPINLOCK(media_jacks_lock);

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

void sl_media_data_jack_del(u8 ldev_num, u8 jack_num)
{
	unsigned long         irq_flags;
	struct sl_media_jack *media_jack;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);
	if (!media_jack) {
		sl_media_log_dbg(NULL, LOG_NAME, "not found (jack_num = %u)", jack_num);
		return;
	}

	spin_lock_irqsave(&media_jacks_lock, irq_flags);
	media_jacks[ldev_num][jack_num] = NULL;
	spin_unlock_irqrestore(&media_jacks_lock, irq_flags);

	sl_media_log_dbg(media_jack, LOG_NAME, "del (jack = 0x%p)", media_jack);

	kfree(media_jack);
}

int sl_media_data_jack_new(struct sl_media_ldev *media_ldev, u8 jack_num)
{
	struct sl_media_jack *media_jack;
	unsigned long         irq_flags;

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

	sl_media_log_dbg(media_jack, LOG_NAME, "new (jack = 0x%p)", media_jack);

	spin_lock_irqsave(&media_jacks_lock, irq_flags);
	media_jacks[media_ldev->num][jack_num] = media_jack;
	spin_unlock_irqrestore(&media_jacks_lock, irq_flags);

	return 0;
}

struct sl_media_jack *sl_media_data_jack_get(u8 ldev_num, u8 jack_num)
{
	unsigned long         irq_flags;
	struct sl_media_jack *media_jack;

	spin_lock_irqsave(&media_jacks_lock, irq_flags);
	media_jack = media_jacks[ldev_num][jack_num];
	spin_unlock_irqrestore(&media_jacks_lock, irq_flags);

	sl_media_log_dbg(media_jack, LOG_NAME, "get (jack = 0x%p)", media_jack);

	return media_jack;
}

void sl_media_data_cable_serdes_settings_clr(struct sl_media_jack *media_jack)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "serdes settings clr");

	memset(&(media_jack->serdes_settings), 0,
			sizeof(struct sl_media_serdes_settings));
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
	unsigned long         irq_flags;
	struct sl_media_lgrp *media_lgrp;

	sl_media_log_dbg(media_jack, LOG_NAME,
		"media attr set (vendor = %d %s, type = 0x%X %s, length_cm = %d, speeds_map = 0x%X)",
		media_attr->vendor, sl_media_vendor_str(media_attr->vendor),
		media_attr->type, sl_media_type_str(media_attr->type),
		media_attr->length_cm, media_attr->speeds_map);

	media_lgrp = sl_media_data_lgrp_get(cable_info->ldev_num, cable_info->lgrp_num);
	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
		sl_media_log_err_trace(media_jack, LOG_NAME, "media attr already set");
		return -EEXIST;
	}

	if (cable_info->fake_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		cable_info->stashed_media_attr = *media_attr;
		cable_info->real_cable_status = CABLE_MEDIA_ATTR_STASHED;
		spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
	} else {
		cable_info->media_attr = *media_attr;
		cable_info->real_cable_status = CABLE_MEDIA_ATTR_ADDED;
		spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
		if (media_lgrp)
			sl_media_data_jack_cable_present_send(media_lgrp);
	}

	return 0;
}

void sl_media_data_jack_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info)
{
	unsigned long         irq_flags;
	struct sl_media_lgrp *media_lgrp;

	sl_media_log_dbg(media_jack, LOG_NAME, "media attr clr");

	media_lgrp = sl_media_data_lgrp_get(cable_info->ldev_num, cable_info->lgrp_num);
	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_REMOVED) {
		spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
		sl_media_log_dbg(media_jack, LOG_NAME, "no media attr to clear");
		return;
	}

	if (cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED)
		memset(&(cable_info->media_attr), 0, sizeof(struct sl_media_attr));

	cable_info->real_cable_status = CABLE_MEDIA_ATTR_REMOVED;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	if (media_lgrp) {
		/* Only send notification if both real and fake cables are removed */
		if (cable_info->fake_cable_status == CABLE_MEDIA_ATTR_REMOVED)
			sl_media_data_jack_cable_not_present_send(media_lgrp);
	}
}

void sl_media_data_jack_cable_present_send(struct sl_media_lgrp *media_lgrp)
{
	int                       rtn;
	unsigned long             irq_flags;
	union sl_lgrp_notif_info  info;

	sl_media_log_dbg(media_lgrp, LOG_NAME, "present send");

	spin_lock_irqsave(&media_lgrp->media_jack->data_lock, irq_flags);
	if (media_lgrp->cable_info->real_cable_status == CABLE_MEDIA_ATTR_ADDED ||
				media_lgrp->cable_info->fake_cable_status == CABLE_MEDIA_ATTR_ADDED) {
		spin_unlock_irqrestore(&media_lgrp->media_jack->data_lock, irq_flags);
		info.media_attr = media_lgrp->cable_info->media_attr;
		rtn = sl_ctl_lgrp_notif_enqueue(sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num),
			SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_PRESENT, &info, 0);
		if (rtn)
			sl_media_log_warn_trace(media_lgrp, LOG_NAME,
				"present_send ctl_lgrp_notif_enqueue failed [%d]", rtn);
		return;
	}
	spin_unlock_irqrestore(&media_lgrp->media_jack->data_lock, irq_flags);
}

void sl_media_data_jack_cable_not_present_send(struct sl_media_lgrp *media_lgrp)
{
	int rtn;

	sl_media_log_dbg(media_lgrp, LOG_NAME, "not present send");

	rtn = sl_ctl_lgrp_notif_enqueue(sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num),
		SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_NOT_PRESENT, NULL, 0);
	if (rtn)
		sl_media_log_warn_trace(media_lgrp, LOG_NAME,
			"not_present_send ctl_lgrp_notif_enqueue failed [%d]", rtn);
}

void sl_media_data_jack_cable_error_send(struct sl_media_lgrp *media_lgrp)
{
	int                      rtn;
	union sl_lgrp_notif_info info;

	sl_media_log_dbg(media_lgrp, LOG_NAME, "error send");

	/*
	 * error info in media_attr.options
	 */
	info.media_attr = media_lgrp->cable_info->media_attr;
	rtn = sl_ctl_lgrp_notif_enqueue(sl_ctl_lgrp_get(media_lgrp->media_ldev->num, media_lgrp->num),
		SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_ERROR, &info, 0);
	if (rtn)
		sl_media_log_warn_trace(media_lgrp, LOG_NAME,
			"error_send ctl_lgrp_notif_enqueue failed [%d]", rtn);
}
