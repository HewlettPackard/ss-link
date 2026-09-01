// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/string.h>

#include <linux/hpe/sl/sl_media.h>
#include <linux/hsnxcvr-api.h>

#include "sl_asic.h"
#include "base/sl_media_log.h"
#include "base/sl_media_eeprom.h"
#include "sl_media_jack.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_ldev.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_cable_db_ops.h"

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

static u32 sl_media_data_jack_insert_status_map[SL_ASIC_MAX_LDEVS];

void sl_media_data_jack_insert_status_map_init(u8 ldev_num)
{
	sl_media_data_jack_insert_status_map[ldev_num] = 0;
}

static void sl_media_data_jack_insert_status_map_set(u8 ldev_num, u8 bit_num)
{
	sl_media_data_jack_insert_status_map[ldev_num] |= BIT(bit_num);
}

void sl_media_data_jack_insert_status_map_clr(u8 ldev_num, u8 bit_num)
{
	sl_media_data_jack_insert_status_map[ldev_num] &= ~BIT(bit_num);
}

u32 sl_media_data_jack_insert_status_map_get(u8 ldev_num)
{
	return sl_media_data_jack_insert_status_map[ldev_num];
}

static int sl_media_data_jack_eeprom_page0_get(struct sl_media_jack *media_jack)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "eeprom page0 get");

	rtn = sl_media_io_read(media_jack, 0, 0, media_jack->eeprom_page0, sizeof(media_jack->eeprom_page0));
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "eeprom page0 get media_io_read failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

#define FLAT_MEM_OFFSET    2
#define CMIS_FLAT_MEM_BIT  7
#define SFF_FLAT_MEM_BIT   2
static int sl_media_data_jack_eeprom_page1_get(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  flat_mem_bit;

	sl_media_log_dbg(media_jack, LOG_NAME, "eeprom page1 get");

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		flat_mem_bit = CMIS_FLAT_MEM_BIT;
	} else if (!sl_media_data_jack_media_is_format_cmis(media_jack)) {
		flat_mem_bit = SFF_FLAT_MEM_BIT;
	} else {
		sl_media_log_err_trace(media_jack, LOG_NAME, "eeprom page1 get unknown cable format");
		return -EMEDIUMTYPE;
	}

	if ((media_jack->eeprom_page0[FLAT_MEM_OFFSET] & BIT(flat_mem_bit)) != 0) {
		sl_media_log_dbg(media_jack, LOG_NAME, "eeprom page1 get no page1");
		return 0;
	}

	rtn = sl_media_io_read(media_jack, 1, 0, media_jack->eeprom_page1, sizeof(media_jack->eeprom_page1));
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "eeprom page1 get media_io_read failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

#define SL_MEDIA_TEMPERATURE_WARN_LIMIT_CELSIUS_MIN     55
#define SL_MEDIA_TEMPERATURE_WARN_LIMIT_CELSIUS_MAX     85
#define SL_MEDIA_TEMPERATURE_WARN_LIMIT_CELSIUS_DEFAULT 70
#define SL_MEDIA_CMIS_TEMP_WARN_LIMIT_PAGE              2
#define SL_MEDIA_CMIS_TEMP_WARN_LIMIT_OFFSET            132
#define SL_MEDIA_SFF_TEMP_WARN_LIMIT_PAGE               3
#define SL_MEDIA_SFF_TEMP_WARN_LIMIT_OFFSET             132
static int sl_media_data_jack_temp_warn_limit_get(struct sl_media_jack *media_jack, u8 *data)
{
	u8  i;
	u8  value;
	u8  page;
	u8  offset;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "temp warn limit get");

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		page   = SL_MEDIA_CMIS_TEMP_WARN_LIMIT_PAGE;
		offset = SL_MEDIA_CMIS_TEMP_WARN_LIMIT_OFFSET;
	} else {
		page   = SL_MEDIA_SFF_TEMP_WARN_LIMIT_PAGE;
		offset = SL_MEDIA_SFF_TEMP_WARN_LIMIT_OFFSET;
	}

	for (i = 0; i < 3; ++i) {
		rtn = sl_media_io_read8(media_jack, page, offset, &value);
		if (rtn)
			continue;

		if (value < SL_MEDIA_TEMPERATURE_WARN_LIMIT_CELSIUS_MIN || value > SL_MEDIA_TEMPERATURE_WARN_LIMIT_CELSIUS_MAX)
			continue;

		*data = value;
		return 0;
	}

	return -EINVAL;
}

#define SL_MEDIA_TEMPERATURE_DOWN_LIMIT_CELSIUS_MIN     65
#define SL_MEDIA_TEMPERATURE_DOWN_LIMIT_CELSIUS_MAX     95
#define SL_MEDIA_TEMPERATURE_DOWN_LIMIT_CELSIUS_DEFAULT 80
#define SL_MEDIA_CMIS_TEMP_DOWN_LIMIT_PAGE              2
#define SL_MEDIA_CMIS_TEMP_DOWN_LIMIT_OFFSET            128
#define SL_MEDIA_SFF_TEMP_DOWN_LIMIT_PAGE               3
#define SL_MEDIA_SFF_TEMP_DOWN_LIMIT_OFFSET             128
static int sl_media_data_jack_temp_down_limit_get(struct sl_media_jack *media_jack, u8 *data)
{
	u8  i;
	u8  value;
	u8  page;
	u8  offset;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "temp down limit get");

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		page   = SL_MEDIA_CMIS_TEMP_DOWN_LIMIT_PAGE;
		offset = SL_MEDIA_CMIS_TEMP_DOWN_LIMIT_OFFSET;
	} else {
		page   = SL_MEDIA_SFF_TEMP_DOWN_LIMIT_PAGE;
		offset = SL_MEDIA_SFF_TEMP_DOWN_LIMIT_OFFSET;
	}

	for (i = 0; i < 3; ++i) {
		rtn = sl_media_io_read8(media_jack, page, offset, &value);
		if (rtn)
			continue;

		if (value < SL_MEDIA_TEMPERATURE_DOWN_LIMIT_CELSIUS_MIN || value > SL_MEDIA_TEMPERATURE_DOWN_LIMIT_CELSIUS_MAX)
			continue;

		*data = value;
		return 0;
	}

	return -EINVAL;
}

static int sl_media_data_jack_cable_setup(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	int rtn;
	int x;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable setup");

	rtn = sl_media_data_jack_eeprom_page0_get(media_jack);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "cable setup eeprom_page0_get failed [%d]", rtn);
		media_attr->errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
		return rtn;
	}

	rtn = sl_media_eeprom_format_get(media_jack, &media_attr->format, &media_attr->version);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "cable setup eeprom_format_get failed [%d]", rtn);
		media_jack->is_cable_format_unsupported = true;
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_EEPROM_FORMAT_UNSUPPORTED);
		media_attr->errors |= SL_MEDIA_ERROR_CABLE_FORMAT_UNSUPPORTED;
		media_attr->errors |= SL_MEDIA_ERROR_TRYABLE;
		return rtn;
	}

	rtn = sl_media_data_jack_eeprom_page1_get(media_jack);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "cable setup eeprom_page1_get failed [%d]", rtn);
		media_attr->errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
		return rtn;
	}

	sl_media_eeprom_parse(media_jack, media_attr);

	if (!SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr->type))
		media_attr->info |= SL_MEDIA_INFO_AUTONEG;

	rtn = sl_media_data_cable_db_ops_cable_validate(media_attr, media_jack);
	if (rtn) {
		/* workaround to read cable info a second time */
		sl_media_log_warn(media_jack, LOG_NAME,
				  "cable validate failed, reading cable info again");
		for (x = 0; x < SL_MEDIA_EEPROM_PAGE_SIZE; x += 16) {
			sl_media_log_dbg(media_jack, LOG_NAME,
					 "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
					 media_jack->eeprom_page0[x],      media_jack->eeprom_page0[x + 1],  media_jack->eeprom_page0[x + 2],
					 media_jack->eeprom_page0[x + 3],  media_jack->eeprom_page0[x + 4],  media_jack->eeprom_page0[x + 5],
					 media_jack->eeprom_page0[x + 6],  media_jack->eeprom_page0[x + 7],  media_jack->eeprom_page0[x + 8],
					 media_jack->eeprom_page0[x + 9],  media_jack->eeprom_page0[x + 10], media_jack->eeprom_page0[x + 11],
					 media_jack->eeprom_page0[x + 12], media_jack->eeprom_page0[x + 13], media_jack->eeprom_page0[x + 14],
					 media_jack->eeprom_page0[x + 15]);
		}
		sl_media_data_jack_data_clr(media_jack);
		sl_media_data_jack_eeprom_page0_get(media_jack);
		sl_media_eeprom_format_get(media_jack, &media_attr->format, &media_attr->version);
		sl_media_data_jack_eeprom_page1_get(media_jack);
		sl_media_eeprom_parse(media_jack, media_attr);
		rtn = sl_media_data_cable_db_ops_cable_validate(media_attr, media_jack);
		if (rtn) {
			sl_media_log_info(media_jack, LOG_NAME,
					  "unsupported cable inserted (hpe_part_num = %s, vendor = %d %s, type = 0x%X %s, length = %u cm)",
					  media_attr->hpe_pn_str, media_attr->vendor, sl_media_vendor_str(media_attr->vendor),
					  media_attr->type, sl_media_type_str(media_attr->type), media_attr->length_cm);
			for (x = 0; x < SL_MEDIA_EEPROM_PAGE_SIZE; x += 16) {
				sl_media_log_dbg(media_jack, LOG_NAME,
						 "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
						 media_jack->eeprom_page0[x],      media_jack->eeprom_page0[x + 1],  media_jack->eeprom_page0[x + 2],
						 media_jack->eeprom_page0[x + 3],  media_jack->eeprom_page0[x + 4],  media_jack->eeprom_page0[x + 5],
						 media_jack->eeprom_page0[x + 6],  media_jack->eeprom_page0[x + 7],  media_jack->eeprom_page0[x + 8],
						 media_jack->eeprom_page0[x + 9],  media_jack->eeprom_page0[x + 10], media_jack->eeprom_page0[x + 11],
						 media_jack->eeprom_page0[x + 12], media_jack->eeprom_page0[x + 13], media_jack->eeprom_page0[x + 14],
						 media_jack->eeprom_page0[x + 15]);
			}
			media_attr->errors |= SL_MEDIA_ERROR_CABLE_UNSUPPORTED;
			media_attr->errors |= SL_MEDIA_ERROR_TRYABLE;
		}
	}
	if (rtn == 0) {
		sl_media_log_info(media_jack, LOG_NAME,
				  "supported cable inserted (hpe_part_num = %d %s, vendor = %d %s, type = 0x%X %s, length = %u cm)",
				  media_attr->hpe_pn, media_attr->hpe_pn_str,
				  media_attr->vendor, sl_media_vendor_str(media_attr->vendor),
				  media_attr->type, sl_media_type_str(media_attr->type),
				  media_attr->length_cm);
	}

	if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr->type) &&
	    !media_jack->is_cable_unsupported && !media_jack->is_supported_ss200_cable) {
		if (!sl_media_eeprom_is_fw_version_supported(media_jack, media_attr)) {
			sl_media_log_warn_trace(media_jack, LOG_NAME, "cable setup fw version unsupported");
			media_attr->errors |= SL_MEDIA_ERROR_CABLE_FW_UNSUPPORTED;
			media_attr->errors |= SL_MEDIA_ERROR_TRYABLE;
		}
		/* disallow BJ100 speed on active cables */
		media_attr->speeds_map &= ~SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
	}

	if (media_jack->is_supported_ss200_cable) {
		media_attr->speeds_map  = 0;
		media_attr->speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
		media_attr->speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_100G;
		media_attr->speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		media_attr->info       |= SL_MEDIA_INFO_SUPPORTED_SS200_CABLE;
	}

	return 0;
}

static int sl_media_data_jack_backplane_setup(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "backplane setup");

	media_attr->vendor     = SL_MEDIA_VENDOR_HPE;
	media_attr->type       = SL_MEDIA_TYPE_BKP;
	media_attr->info       = SL_MEDIA_INFO_AUTONEG;
	media_attr->length_cm  = 25;
	media_attr->hpe_pn     = 0x42434B50; /* "BCKP" in ASCII */
	media_attr->furcation  = SL_MEDIA_FURCATION_X1;
	media_attr->speeds_map = SL_MEDIA_SPEEDS_SUPPORT_CK_400G |
				 SL_MEDIA_SPEEDS_SUPPORT_CK_200G |
				 SL_MEDIA_SPEEDS_SUPPORT_BS_200G |
				 SL_MEDIA_SPEEDS_SUPPORT_CK_100G |
				 SL_MEDIA_SPEEDS_SUPPORT_CD_100G |
				 SL_MEDIA_SPEEDS_SUPPORT_BJ_100G |
				 SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
	media_attr->max_speed  = SL_MEDIA_SPEEDS_SUPPORT_CK_400G;

	strncpy(media_attr->serial_num_str, "AK20212120", sizeof(media_attr->serial_num_str));
	strncpy(media_attr->hpe_pn_str,     "BACKPLANE",  sizeof(media_attr->hpe_pn_str));
	strncpy(media_attr->date_code_str,  "08-19-21",   sizeof(media_attr->date_code_str));

	memset(media_attr->fw_ver, 0, sizeof(media_attr->fw_ver));

	return 0;
}

static int sl_media_data_jack_active_cable_setup(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	int rtn;
	u8  value;

	sl_media_log_dbg(media_jack, LOG_NAME, "active cable setup");

	rtn = sl_media_data_jack_temp_down_limit_get(media_jack, &value);
	if (rtn) {
		sl_media_log_warn_trace(media_jack, LOG_NAME,
					"active cable setup temp_down_limit_get failed [%d]", rtn);
		media_jack->temperature_down_limit_c = SL_MEDIA_TEMPERATURE_DOWN_LIMIT_CELSIUS_DEFAULT;
		media_attr->errors |= SL_MEDIA_ERROR_TEMP_DOWN_LIMIT_DEFAULT;
		media_attr->errors |= SL_MEDIA_ERROR_TRYABLE;
	} else {
		media_jack->temperature_down_limit_c = value;
	}

	rtn = sl_media_data_jack_temp_warn_limit_get(media_jack, &value);
	if (rtn) {
		sl_media_log_warn_trace(media_jack, LOG_NAME,
					"active cable setup temp_warn_limit_get failed [%d]", rtn);
		media_jack->temperature_warn_limit_c = SL_MEDIA_TEMPERATURE_WARN_LIMIT_CELSIUS_DEFAULT;
		media_attr->errors |= SL_MEDIA_ERROR_TEMP_WARN_LIMIT_DEFAULT;
		media_attr->errors |= SL_MEDIA_ERROR_TRYABLE;
	} else {
		media_jack->temperature_warn_limit_c = value;
	}

	rtn = sl_media_data_jack_cable_soft_reset(media_jack);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "active cable setup cable_soft_reset failed [%d]", rtn);
		media_attr->errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
		return rtn;
	}

	/* must be after the cable reset */
	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "active cable setup cable_high_power_set failed [%d]", rtn);
		media_attr->errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
		return rtn;
	}

	if (sl_media_data_jack_cable_hw_shift_state_get(media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED)
		sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_DOWNSHIFTED);
	else if (sl_media_data_jack_cable_hw_shift_state_get(media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED)
		sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_UPSHIFTED);
	else
		sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_NOTSHIFTED);

	sl_media_log_dbg(media_jack, LOG_NAME, "active cable setup done");

	return 0;
}

int sl_media_data_jack_insert(struct sl_media_jack *media_jack)
{
	int                  rtn;
	int                  ret;
	struct sl_media_attr media_attr;

	sl_media_log_dbg(media_jack->media_ldev, LOG_NAME,
			 "insert (ldev = 0x%p, jack = 0x%p, jack_num = %u)",
			 media_jack->media_ldev, media_jack, media_jack->num);

	sl_media_jack_fault_cause_clr(media_jack);

	memset(&media_attr, 0, sizeof(struct sl_media_attr));
	media_attr.magic  = SL_MEDIA_ATTR_MAGIC;
	media_attr.ver    = SL_MEDIA_ATTR_VER;
	media_attr.size   = sizeof(struct sl_media_attr);
	media_attr.errors = 0;
	media_attr.info   = 0;

	switch (media_jack->jack_type) {
	case XCVR_JACK_BACKPLANE:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_BACKPLANE;
		rtn = sl_media_data_jack_backplane_setup(media_jack, &media_attr);
		break;
	case XCVR_JACK_SFP:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_SFP;
		rtn = sl_media_data_jack_cable_setup(media_jack, &media_attr);
		break;
	case XCVR_JACK_QSFP:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_QSFP;
		media_attr.jack_type_info.qsfp.density = SL_MEDIA_QSFP_DENSITY_SINGLE;
		rtn = sl_media_data_jack_cable_setup(media_jack, &media_attr);
		break;
	case XCVR_JACK_QSFPDD:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_QSFP;
		media_attr.jack_type_info.qsfp.density = SL_MEDIA_QSFP_DENSITY_DOUBLE;
		rtn = sl_media_data_jack_cable_setup(media_jack, &media_attr);
		break;
	case XCVR_JACK_OSFP:
	case XCVR_JACK_OSFPXD:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_OSFP;
		rtn = sl_media_data_jack_cable_setup(media_jack, &media_attr);
		break;
	default:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_UNSUPPORTED;
		sl_media_log_warn(media_jack, LOG_NAME, "unsupported jack");
	}

	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "insert cable_setup failed [%d]", rtn);
		sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_CABLE_SETUP);
		goto out;
	}

	rtn = sl_media_data_cable_db_ops_serdes_settings_get(media_jack, &media_attr);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "insert serdes_settings_get failed [%d]", rtn);
		sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SERDES_SETTINGS_GET);
		goto out;
	}

	if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type)) {
		rtn = sl_media_data_jack_active_cable_setup(media_jack, &media_attr);
		if (rtn) {
			sl_media_log_err(media_jack, LOG_NAME, "insert active_cable_setup failed [%d]", rtn);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_ACTIVE_CABLE_SETUP);
			goto out;
		}
	}

	sl_media_data_jack_cable_temp_state_init(media_jack);

	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_INSERTED);

	rtn = 0;

out:

	sl_media_data_jack_last_cable_insert_set(media_jack, &media_attr);

	ret = sl_media_data_jack_cable_attr_set(media_jack, &media_attr);
	if (ret)
		sl_media_log_warn_trace(media_jack->media_ldev, LOG_NAME,
					"insert cable_attr_set failed [%d] (jack_num = %u)",
					ret, media_jack->num);

	sl_media_data_jack_led_set(media_jack);

	sl_media_data_jack_insert_status_map_set(media_jack->media_ldev->num, media_jack->num);

	sl_media_data_jack_cable_attr_send(media_jack);

	sl_media_log_dbg(media_jack, LOG_NAME, "insert done (jack = 0x%p)", media_jack);

	return rtn;
}

void sl_media_data_jack_remove(struct sl_media_jack *media_jack)
{
	u8 i;

	for (i = 0; i < media_jack->port_count; ++i) {
		sl_media_data_jack_media_attr_clr(media_jack, &media_jack->cable_info[i]);
		sl_media_data_jack_cable_hot_notif_sent_set(media_jack, &media_jack->cable_info[i], false);
		sl_media_data_jack_cable_warm_notif_sent_set(media_jack, &media_jack->cable_info[i], false);
		sl_media_data_jack_cable_cold_notif_sent_set(media_jack, &media_jack->cable_info[i], false);
	}

	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_REMOVED);
	sl_media_data_cable_serdes_settings_clr(media_jack);
	sl_media_data_jack_eeprom_clr(media_jack);
	sl_media_data_jack_data_clr(media_jack);
	sl_media_jack_fault_cause_clr(media_jack);
	sl_media_data_jack_led_set(media_jack);

	sl_media_log_info(media_jack, LOG_NAME, "cable removed");
}

static bool ignore_events;

void sl_media_data_jack_event_ignore_set(bool val)
{
	ignore_events = val;
}

#define SL_MEDIA_XCVR_REMOVED (HSNXCVR_EVENT_OFFLINE | HSNXCVR_EVENT_REMOVE)
int sl_media_data_jack_event(struct notifier_block *event_notifier,
			     unsigned long events, void *hdl)
{
	int                       rtn;
	struct xcvr_jack_data_v3  jack_data;
	u8                        jack_num;
	struct sl_media_jack     *media_jack;

	sl_media_log_info(NULL, LOG_NAME, "event (events = 0x%08lX)", events);

	if (ignore_events) {
		sl_media_log_dbg(NULL, LOG_NAME, "event ignore set");
		return NOTIFY_OK;
	}

	rtn = hsnxcvr_jack_v3_get(hdl, &jack_data);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "event jack_get failed [%d]", rtn);
		return NOTIFY_OK;
	}

	rtn = kstrtou8(jack_data.name + 1, 10, &jack_num);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "event kstrtou8 failed [%d]", rtn);
		return NOTIFY_OK;
	}

	if (jack_num >= 201) /* GX backplanes */
		jack_num -= 177;
	else if (jack_num >= 100) /* backplanes */
		jack_num -= 76;
	else
		jack_num -= 1;

	// FIXME: ldev num hardcoded to 0
	media_jack = sl_media_data_jack_get(0, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "event (jack_num = %u)", jack_num);

	if ((events & SL_MEDIA_XCVR_REMOVED) == SL_MEDIA_XCVR_REMOVED)
		sl_media_data_jack_remove(media_jack);

	if (events & HSNXCVR_EVENT_ONLINE)
		sl_media_data_jack_insert(media_jack);

	return NOTIFY_OK;
}
