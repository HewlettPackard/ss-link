// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/sl_media.h>
#include <linux/string.h>

#include "sl_asic.h"
#include "sl_sysfs.h"
#include "sl_media_jack.h"
#include "sl_media_jack_cassini.h"
#include "sl_media_lgrp.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_cable_db_ops.h"
#include "base/sl_media_log.h"
#include "base/sl_media_eeprom.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

#define FLAT_MEM_OFFSET 2
#define FLAT_MEM_BIT    7
static int sl_media_data_jack_eeprom_page1_get(struct sl_media_jack *media_jack, u8 *eeprom_page1)
{
	u8  flat_mem;

	sl_media_log_dbg(media_jack, LOG_NAME, "eeprom page1 get");

	flat_mem = media_jack->eeprom_page0[FLAT_MEM_OFFSET];
	if (test_bit(FLAT_MEM_BIT, (unsigned long *)&flat_mem)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "no page1 in eeprom");
		return 0;
	}

	memcpy(media_jack->eeprom_page1, eeprom_page1, SL_MEDIA_EEPROM_PAGE_SIZE);

	return 0;
}

int sl_media_jack_cable_insert(u8 ldev_num, u8 lgrp_num, u8 jack_num,
			       u8 *eeprom_page0, u8 *eeprom_page1, u32 flags)
{
	int                        rtn;
	struct sl_media_attr       media_attr;
	struct sl_media_jack      *media_jack;
	unsigned long              irq_flags;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);
	if (!media_jack) {
		sl_media_log_err(NULL, LOG_NAME, "media_data_jack_get failed");
		return -EFAULT;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable insert");

	memset(&media_attr, 0, sizeof(struct sl_media_attr));

	if (flags & SL_MEDIA_TYPE_BACKPLANE) {
		media_attr.magic         = SL_MEDIA_ATTR_MAGIC;
		media_attr.ver           = SL_MEDIA_ATTR_VER;
		media_attr.vendor        = SL_MEDIA_VENDOR_HPE;
		media_attr.type          = SL_MEDIA_TYPE_BKP;
		media_attr.options       = SL_MEDIA_OPT_AUTONEG;
		media_attr.length_cm     = 25;
		media_attr.hpe_pn        = 60821555;
		media_attr.furcation     = SL_MEDIA_FURCATION_X1;
		media_attr.speeds_map    = SL_MEDIA_SPEEDS_SUPPORT_CK_400G |
					   SL_MEDIA_SPEEDS_SUPPORT_CK_200G |
					   SL_MEDIA_SPEEDS_SUPPORT_BS_200G |
					   SL_MEDIA_SPEEDS_SUPPORT_CK_100G |
					   SL_MEDIA_SPEEDS_SUPPORT_CD_100G |
					   SL_MEDIA_SPEEDS_SUPPORT_BJ_100G |
					   SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		media_attr.max_speed     = SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		media_attr.jack_type     = SL_MEDIA_JACK_TYPE_BACKPLANE;
		strncpy(media_attr.serial_num, "AK20212120", sizeof(media_attr.serial_num));
		strncpy(media_attr.hpe_pn_str, "PK60821-555", sizeof(media_attr.hpe_pn_str));
	} else {
		memcpy(media_jack->eeprom_page0, eeprom_page0, SL_MEDIA_EEPROM_PAGE_SIZE);
		sl_media_data_jack_eeprom_page1_get(media_jack, eeprom_page1);
		rtn = sl_media_eeprom_parse(media_jack, &media_attr);
		if (rtn) {
			sl_media_log_err(media_jack, LOG_NAME, "eeprom_parse failed [%d]", rtn);
			if (sl_media_jack_is_cable_format_invalid(media_jack)) {
				memset(&media_attr, 0, sizeof(struct sl_media_attr));
				media_attr.options |= SL_MEDIA_OPT_CABLE_FORMAT_INVALID;
				rtn = sl_media_data_jack_media_attr_set(media_jack, &media_jack->cable_info[0],
									&media_attr);
				if (rtn) {
					sl_media_log_err(media_jack, LOG_NAME, "media attr set failed [%d]", rtn);
					return rtn;
				}
			}
			return -EFAULT;
		}
		rtn = sl_media_data_cable_db_ops_cable_validate(&media_attr, media_jack);
		if (rtn) {
			sl_media_log_warn(media_jack, LOG_NAME, "cable validate failed [%d]", rtn);
			sl_media_log_warn(media_jack, LOG_NAME,
				"media attrs (vendor = %d %s, type = 0x%X %s, length_cm = %d, speeds = 0x%X)",
				media_attr.vendor, sl_media_vendor_str(media_attr.vendor),
				media_attr.type, sl_media_type_str(media_attr.type),
				media_attr.length_cm, media_attr.speeds_map);
			media_jack->is_cable_not_supported = true;
			media_attr.options |= SL_MEDIA_OPT_CABLE_NOT_SUPPORTED;
		}
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_QSFP;
		media_attr.jack_type_info.qsfp.density = SL_MEDIA_QSFP_DENSITY_SINGLE;
	}

	if (media_jack->is_cable_not_supported)
		flags |= SL_MEDIA_TYPE_NOT_SUPPORTED;

	if (media_attr.vendor == SL_MEDIA_VENDOR_MULTILANE)
		flags |= SL_MEDIA_TYPE_LOOPBACK;

	rtn = sl_media_data_cable_db_ops_serdes_settings_get(media_jack, flags);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "serdes settings get failed [%d]", rtn);
		return rtn;
	}

	/*
	 * only first element is valid in cable_info since single lgrp on cassini
	 */
	rtn = sl_media_data_jack_media_attr_set(media_jack, &media_jack->cable_info[0], &media_attr);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media attr set failed [%d]", rtn);
		return rtn;
	}

	/*
	 * kobj cleanup in the callee function
	 */
	rtn = sl_sysfs_media_speeds_create(ldev_num, lgrp_num);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media speeds create failed [%d]", rtn);
		return rtn;
	}

	if (media_attr.type == SL_MEDIA_TYPE_AOC ||
		media_attr.type == SL_MEDIA_TYPE_AEC) {
		rtn = sl_media_jack_cable_high_power_set(ldev_num, jack_num);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "high power set failed [%d]", rtn);
			return rtn;
		}
	}

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->state = SL_MEDIA_JACK_CABLE_ONLINE;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	return 0;
}

int sl_media_jack_cable_remove(u8 ldev_num, u8 lgrp_num, u8 jack_num)
{
	struct sl_media_jack      *media_jack;
	unsigned long              irq_flags;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);
	if (!media_jack) {
		sl_media_log_err(NULL, LOG_NAME, "media_data_jack_get failed");
		return -EFAULT;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable remove");

	/*
	 * only first element is valid in cable_info since single lgrp on cassini
	 */
	sl_media_data_jack_media_attr_clr(media_jack, &media_jack->cable_info[0]);
	sl_media_data_cable_serdes_settings_clr(media_jack);
	sl_media_data_jack_eeprom_clr(media_jack);

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->state = SL_MEDIA_JACK_CABLE_REMOVED;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	return 0;
}
