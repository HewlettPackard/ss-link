// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/string.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_asic.h"
#include "sl_sysfs.h"
#include "sl_media_jack.h"
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
	sl_media_log_dbg(media_jack, LOG_NAME, "eeprom page1 get");

	if ((media_jack->eeprom_page0[FLAT_MEM_OFFSET] & BIT(FLAT_MEM_BIT)) != 0) {
		sl_media_log_dbg(media_jack, LOG_NAME, "no page1 in eeprom");
		return 0;
	}

	memcpy(media_jack->eeprom_page1, eeprom_page1, SL_MEDIA_EEPROM_PAGE_SIZE);

	return 0;
}

static int sl_media_jack_cable_attr_set(struct sl_media_jack *media_jack, u8 ldev_num, u8 lgrp_num,
					struct sl_media_attr *media_attr)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable attr set");

	/*
	 * only first element is valid in cable_info since single lgrp on nic
	 */
	media_jack->cable_info[0].ldev_num = ldev_num;
	media_jack->cable_info[0].lgrp_num = lgrp_num;

	rtn = sl_media_data_jack_media_attr_set(media_jack, &media_jack->cable_info[0], media_attr);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "media attr set failed [%d]", rtn);
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_MEDIA_ATTR_SET);
		return rtn;
	}

	return 0;
}

int sl_media_jack_cable_insert(u8 ldev_num, u8 lgrp_num, u8 jack_num,
			       u8 *eeprom_page0, u8 *eeprom_page1, u32 flags)
{
	int                   rtn;
	struct sl_media_attr  media_attr;
	struct sl_media_jack *media_jack;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);
	if (!media_jack) {
		sl_media_log_err(NULL, LOG_NAME, "media_data_jack_get failed");
		return -EFAULT;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable insert");

	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_NONE);

	memset(&media_attr, 0, sizeof(struct sl_media_attr));

	media_attr.magic    = SL_MEDIA_ATTR_MAGIC;
	media_attr.ver      = SL_MEDIA_ATTR_VER;
	media_attr.size     = sizeof(struct sl_media_attr);
	media_attr.errors   = 0;
	media_attr.info     = 0;

	if (flags & SL_MEDIA_TYPE_BACKPLANE) {
		media_attr.vendor        = SL_MEDIA_VENDOR_HPE;
		media_attr.type          = SL_MEDIA_TYPE_BKP;
		media_attr.info          = SL_MEDIA_INFO_AUTONEG;
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
		strncpy(media_attr.serial_num_str, "AK20212120", sizeof(media_attr.serial_num_str));
		strncpy(media_attr.hpe_pn_str, "PK60821-555", sizeof(media_attr.hpe_pn_str));
		strncpy(media_attr.date_code_str, "08-19-21", sizeof(media_attr.date_code_str));
		memset(media_attr.fw_ver, 0, sizeof(media_attr.fw_ver));
	} else {
		memcpy(media_jack->eeprom_page0, eeprom_page0, SL_MEDIA_EEPROM_PAGE_SIZE);

		sl_media_data_jack_eeprom_page1_get(media_jack, eeprom_page1);

		rtn = sl_media_eeprom_format_get(media_jack, &(media_attr.format));
		if (rtn) {
			sl_media_log_warn_trace(media_jack, LOG_NAME, "eeprom format invalid");
			memset(&media_attr, 0, sizeof(struct sl_media_attr));
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_FORMAT_INVALID;
			media_attr.errors |= SL_MEDIA_ERROR_TRYABLE;
			rtn = sl_media_jack_cable_attr_set(media_jack, ldev_num, lgrp_num, &media_attr);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", rtn);
			return 0;
		}

		sl_media_eeprom_parse(media_jack, &media_attr);

		if (media_attr.type == SL_MEDIA_TYPE_PEC)
			media_attr.info |= SL_MEDIA_INFO_AUTONEG;

		rtn = sl_media_data_cable_db_ops_cable_validate(&media_attr, media_jack);
		if (rtn) {
			sl_media_log_warn_trace(media_jack, LOG_NAME,
						"cable validate failed [%d] (vendor = %d %s, type = 0x%X %s, length_cm = %d, speeds = 0x%X)",
						rtn, media_attr.vendor, sl_media_vendor_str(media_attr.vendor),
						media_attr.type, sl_media_type_str(media_attr.type),
						media_attr.length_cm, media_attr.speeds_map);

			media_jack->is_cable_not_supported = true;
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_NOT_SUPPORTED;
			media_attr.errors |= SL_MEDIA_ERROR_TRYABLE;
		}

		if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type) &&
		    !media_jack->is_cable_not_supported && !media_jack->is_supported_ss200_cable) {
			if (!sl_media_eeprom_is_fw_version_valid(media_jack, &media_attr)) {
				sl_media_log_warn_trace(media_jack, LOG_NAME, "eeprom fw version invalid");
				media_attr.errors |= SL_MEDIA_ERROR_CABLE_FW_INVALID;
				media_attr.errors |= SL_MEDIA_ERROR_TRYABLE;
			}
			/*
			 * disallow BJ100 speed on active cables
			 */
			media_attr.speeds_map &= ~SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		}

		if (media_jack->is_supported_ss200_cable) {
			media_attr.speeds_map  = 0;
			media_attr.speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
			media_attr.speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_100G;
			media_attr.speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
			media_attr.info       |= SL_MEDIA_INFO_SUPPORTED_SS200_CABLE;
		}

		media_attr.jack_type = SL_MEDIA_JACK_TYPE_QSFP;
		media_attr.jack_type_info.qsfp.density = SL_MEDIA_QSFP_DENSITY_SINGLE;
	}

	if (media_jack->is_cable_not_supported)
		flags |= SL_MEDIA_TYPE_NOT_SUPPORTED;

	if (media_attr.vendor == SL_MEDIA_VENDOR_MULTILANE)
		flags |= SL_MEDIA_TYPE_LOOPBACK;

	rtn = sl_media_data_cable_db_ops_serdes_settings_get(media_jack, media_attr.type, flags);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SERDES_SETTINGS_GET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "serdes settings get failed [%d]", rtn);
		rtn = sl_media_jack_cable_attr_set(media_jack, ldev_num, lgrp_num, &media_attr);
		if (rtn)
			sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", rtn);
		return 0;
	}

	if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type)) {
		rtn = sl_media_data_jack_cable_soft_reset(media_jack);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "cable soft reset failed [%d]", rtn);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			rtn = sl_media_jack_cable_attr_set(media_jack, ldev_num, lgrp_num, &media_attr);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", rtn);
			return rtn;
		}

		rtn = sl_media_jack_cable_high_power_set(ldev_num, jack_num);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "high power set failed [%d]", rtn);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			rtn = sl_media_jack_cable_attr_set(media_jack, ldev_num, lgrp_num, &media_attr);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", rtn);
			return 0;
		}
	}

	rtn = sl_media_jack_cable_attr_set(media_jack, ldev_num, lgrp_num, &media_attr);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", rtn);
		return 0;
	}

	/*
	 * kobj cleanup in the callee function
	 */
	rtn = sl_sysfs_media_speeds_create(ldev_num, lgrp_num);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "media speeds create failed [%d]", rtn);
		return rtn;
	}

	if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type)) {
		if (sl_media_data_jack_cable_hw_shift_state_get(media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED)
			sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_DOWNSHIFTED);
		else if (sl_media_data_jack_cable_hw_shift_state_get(media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED)
			sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_UPSHIFTED);
		else
			sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_INVALID);
	}

	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ONLINE);

	return 0;
}

int sl_media_jack_cable_remove(u8 ldev_num, u8 lgrp_num, u8 jack_num)
{
	struct sl_media_jack *media_jack;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);
	if (!media_jack) {
		sl_media_log_err(NULL, LOG_NAME, "media_data_jack_get failed");
		return -EFAULT;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable remove");

	/*
	 * only first element is valid in cable_info since single lgrp on nic
	 */
	sl_media_data_jack_media_attr_clr(media_jack, &media_jack->cable_info[0]);
	sl_media_data_cable_serdes_settings_clr(media_jack);
	sl_media_data_jack_eeprom_clr(media_jack);

	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_REMOVED);
	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_NONE);

	return 0;
}

bool sl_media_jack_cable_is_high_temp_set(struct sl_media_jack *media_jack)
{
	return false;
}

void sl_media_jack_cable_is_high_temp_clr(struct sl_media_jack *media_jack)
{
}

void sl_media_jack_cable_high_temp_notif_send(struct sl_media_jack *media_jack)
{
}

void sl_media_jack_cable_high_temp_notif_sent_set(struct sl_media_jack *media_jack,
						  struct sl_media_lgrp_cable_info *cable_info, bool value)
{
}
