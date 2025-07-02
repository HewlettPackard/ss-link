// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <linux/sl_media.h>

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_media_eeprom.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "base/sl_media_log.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_jack_rosetta.h"
#include "data/sl_media_data_jack_cassini.h"

#define LOG_NAME SL_MEDIA_EEPROM_LOG_NAME

#define IDENTIFIER_OFFSET    0
#define REV_CMPL_OFFSET      1
static int sl_media_eeprom_format_get(struct sl_media_jack *media_jack, u8 *format)
{
	if ((media_jack->eeprom_page0[IDENTIFIER_OFFSET] == 0x0D) ||
	    (media_jack->eeprom_page0[IDENTIFIER_OFFSET] == 0x11)) {
		if (media_jack->eeprom_page0[REV_CMPL_OFFSET] < 0x27) {
			*format = SL_MEDIA_MGMT_IF_SFF8636;
			return 0;
		}
	}

	if ((media_jack->eeprom_page0[IDENTIFIER_OFFSET] == 0x18) ||
	    (media_jack->eeprom_page0[IDENTIFIER_OFFSET] == 0x19) ||
	    ((media_jack->eeprom_page0[IDENTIFIER_OFFSET] >= 0x1E) &&
	    (media_jack->eeprom_page0[IDENTIFIER_OFFSET] <= 0x25))) {
		if (((media_jack->eeprom_page0[REV_CMPL_OFFSET] & 0xF0) == 0x30) ||
		    ((media_jack->eeprom_page0[REV_CMPL_OFFSET] & 0xF0) == 0x40) ||
		    ((media_jack->eeprom_page0[REV_CMPL_OFFSET] & 0xF0) == 0x50)) {
			*format = SL_MEDIA_MGMT_IF_CMIS;
			return 0;
		}
	}

	media_jack->is_cable_format_invalid = true;

	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_EEPROM_FORMAT_INVALID);
	sl_media_log_err(media_jack, LOG_NAME,
		"invalid format (id = 0x%X, rev = 0x%X)",
		media_jack->eeprom_page0[IDENTIFIER_OFFSET], media_jack->eeprom_page0[REV_CMPL_OFFSET]);

	return -EMEDIUMTYPE;
}

static void sl_media_eeprom_appsel_info_store(struct sl_media_jack *media_jack, u8 host_interface,
					      u32 *speeds_map, u8 appsel_no, u8 lane_count)
{
	switch (host_interface) {
	case SL_MEDIA_SS1_HOST_INTERFACE_50GAUI_1_C2M:
	case SL_MEDIA_SS1_HOST_INTERFACE_50GBASE_CR:
		media_jack->is_ss200_cable = true;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_100GAUI_2_C2M:
	case SL_MEDIA_SS1_HOST_INTERFACE_100GBASE_CR2:
		media_jack->is_ss200_cable = true;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_100G;
		break;
	case SL_MEDIA_SS2_HOST_INTERFACE_100GBASE_CR1:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		fallthrough;
	case SL_MEDIA_SS2_HOST_INTERFACE_100GAUI_1_L_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_100G;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_200GBASE_CR4:
		media_jack->is_ss200_cable = true;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_200GAUI_4_C2M:
		media_jack->is_ss200_cable = true;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
		media_jack->appsel_no_200_gaui = appsel_no;
		media_jack->lane_count_200_gaui = lane_count;
		media_jack->host_interface_200_gaui = host_interface;
		break;
	case SL_MEDIA_SS2_HOST_INTERFACE_200GBASE_CR2:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		fallthrough;
	case SL_MEDIA_SS2_HOST_INTERFACE_200GAUI_2_S_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_200G;
		break;
	case SL_MEDIA_SS2_HOST_INTERFACE_400GBASE_CR4:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		break;
	case SL_MEDIA_SS2_HOST_INTERFACE_400GAUI_4_S_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		if (media_jack->appsel_no_400_gaui) /* we prefer 4_l_c2m over 4_s_c2m for default ck400G speed */
			break;
		media_jack->appsel_no_400_gaui = appsel_no;
		media_jack->lane_count_400_gaui = lane_count;
		media_jack->host_interface_400_gaui = host_interface;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_400GAUI_4_L_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		media_jack->appsel_no_400_gaui = appsel_no;
		media_jack->lane_count_400_gaui = lane_count;
		media_jack->host_interface_400_gaui = host_interface;
		fallthrough;
	case SL_MEDIA_SS1_HOST_INTERFACE_400GBASE_CR8:
		media_jack->is_ss200_cable = true;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
		fallthrough;
	case SL_MEDIA_SS1_HOST_INTERFACE_CAUI_4_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		break;
	case SL_MEDIA_SS2_HOST_INTERFACE_800GBASE_CR8:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_800G;
		break;
	case SL_MEDIA_SS2_HOST_INTERFACE_800GAUI_8_S_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_800G;
		if (!media_jack->host_interface_400_gaui) {
			media_jack->appsel_no_400_gaui = appsel_no;
			media_jack->host_interface_400_gaui = host_interface;
		}
		break;
	default:
		sl_media_log_dbg(media_jack, LOG_NAME,
				"invalid host interface (0x%x)", host_interface);
	}
}

#define APPSEL_PAGE0_START_OFFSET 86
#define APPSEL_PAGE1_START_OFFSET 223
#define APPSEL_STRIDE             4
#define APPSEL_END                0xFF
#define APPSEL_LAST_PAGE0         114
#define APPSEL_LAST_PAGE1         247
#define APPSEL_LANE_COUNT_OFFSET  2
static int sl_media_eeprom_appsel_info_get(struct sl_media_jack *media_jack, u32 *speeds_map)
{
	u8  host_interface;
	u8  appsel_curr;
	u8  appsel_no;

	appsel_curr = APPSEL_PAGE0_START_OFFSET;
	appsel_no = 1;
	while (appsel_curr <= APPSEL_LAST_PAGE0) {
		host_interface = media_jack->eeprom_page0[appsel_curr];
		if (host_interface == APPSEL_END)
			return 0;
		sl_media_eeprom_appsel_info_store(media_jack, host_interface, speeds_map, appsel_no,
						  media_jack->eeprom_page0[appsel_curr + APPSEL_LANE_COUNT_OFFSET]);
		appsel_curr += APPSEL_STRIDE;
		appsel_no++;
	}

	appsel_curr = APPSEL_PAGE1_START_OFFSET;
	while (appsel_curr <= APPSEL_LAST_PAGE1) {
		host_interface = media_jack->eeprom_page1[appsel_curr];
		if (host_interface == APPSEL_END)
			return 0;
		sl_media_eeprom_appsel_info_store(media_jack, host_interface, speeds_map, appsel_no,
						  media_jack->eeprom_page1[appsel_curr + APPSEL_LANE_COUNT_OFFSET]);
		appsel_curr += APPSEL_STRIDE;
		appsel_no++;
	}

	return 0;
}

static int sl_media_eeprom_furcation_get(struct sl_media_jack *media_jack, u32 *furcation)
{
	// FIXME: temp hack to always return unfurcated
	*furcation = SL_MEDIA_FURCATION_X1;
	return 0;
	// FIXME: get correct answer from eeporm apsel table
}

#define CMIS_TYPE_OFFSET    212
#define SFF8436_TYPE_OFFSET 147
#define CMIS_MEDIA_AOC      0x00
#define CMIS_MEDIA_PEC      0x0A
#define CMIS_MEDIA_AEC      0x0C
#define CMIS_MEDIA_ACC      0x0F
static int sl_media_eeprom_type_get(struct sl_media_jack *media_jack, u8 format, u32 *type)
{
	u8 media;

	if (format == SL_MEDIA_MGMT_IF_CMIS)
		media = media_jack->eeprom_page0[CMIS_TYPE_OFFSET];
	else
		media = media_jack->eeprom_page0[SFF8436_TYPE_OFFSET];

	if (media == CMIS_MEDIA_AOC)
		*type = SL_MEDIA_TYPE_AOC;
	else if (media == CMIS_MEDIA_PEC)
		*type = SL_MEDIA_TYPE_PEC;
	else if (media == CMIS_MEDIA_AEC)
		*type = SL_MEDIA_TYPE_AEC;
	else if (media == CMIS_MEDIA_ACC)
		*type = SL_MEDIA_TYPE_ACC;

	return 0;
}

#define CMIS_VENDOR_OFFSET      129
#define SFF8436_VENDOR_OFFSET   148
#define VENDOR_NAME_MAX_LEN     32
struct vendor {
	char name[VENDOR_NAME_MAX_LEN];
	u32  type;
};
static const struct vendor vendor_list[] = {
	{ .name = "TE Connectivity",
	  .type = SL_MEDIA_VENDOR_TE },
	{ .name = "Hisense",
	  .type = SL_MEDIA_VENDOR_HISENSE },
	{ .name = "FINISAR CORP",
	  .type = SL_MEDIA_VENDOR_FINISAR },
	{ .name = "Bizlink",
	  .type = SL_MEDIA_VENDOR_BIZLINK },
	{ .name = "Cloud Light",
	  .type = SL_MEDIA_VENDOR_CLOUD_LIGHT },
	{ .name = "Molex",
	  .type = SL_MEDIA_VENDOR_MOLEX },
	{ .name = "MULTILANE",
	  .type = SL_MEDIA_VENDOR_MULTILANE },
	{ .name = "Amphenol",
	  .type = SL_MEDIA_VENDOR_AMPHENOL },
};
static int sl_media_eeprom_vendor_get(struct sl_media_jack *media_jack, u8 format, u32 *vendor)
{
	const char *vendor_ptr;
	int         x;

	if (format == SL_MEDIA_MGMT_IF_CMIS)
		vendor_ptr = &(media_jack->eeprom_page0[CMIS_VENDOR_OFFSET]);
	else
		vendor_ptr = &(media_jack->eeprom_page0[SFF8436_VENDOR_OFFSET]);

	for (x = 0; x < ARRAY_SIZE(vendor_list); ++x) {
		if (strnstr(vendor_ptr, vendor_list[x].name, VENDOR_NAME_MAX_LEN) == NULL)
			continue;
		*vendor = vendor_list[x].type;
		return 0;
	}

	*vendor = 0;

	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_EEPROM_VENDOR_INVALID);
	sl_media_log_err_trace(media_jack, LOG_NAME, "eeprom vendor get invalid");

	return -EFAULT;
}

static bool is_valid_char(char c)
{
	return c >= 33 && c <= 126;
}

#define HPE_PN_OFFSET 228
static int sl_media_eeprom_hpe_pn_get(struct sl_media_jack *media_jack, u32 *hpe_pn, char *hpe_pn_str)
{
	const char *pn_ptr;
	int         i;
	int         counter;

	counter = 0;
	for (i = 0; i < SL_MEDIA_HPE_PN_SIZE; ++i) {
		if (is_valid_char(media_jack->eeprom_page0[HPE_PN_OFFSET + i])) {
			hpe_pn_str[counter] = media_jack->eeprom_page0[HPE_PN_OFFSET + i];
			counter++;
		}
	}
	hpe_pn_str[counter] = '\0';

	pn_ptr = &(media_jack->eeprom_page0[HPE_PN_OFFSET]);
	for (i = 0; i < SL_MEDIA_HPE_PN_SIZE - 1; ++i) {
		if (pn_ptr[i] < 48 || pn_ptr[i] > 57) /* if the PN character is not numeric, ignore it*/
			continue;
		*hpe_pn = *hpe_pn * 10 + (pn_ptr[i] - '0');
	}

	return 0;
}

#define SERIAL_NUM_OFFSET 166
static int sl_media_eeprom_serial_num_get(struct sl_media_jack *media_jack, char *serial_num_str)
{
	int i;
	int counter;

	counter = 0;
	for (i = 0; i < SL_MEDIA_SERIAL_NUM_SIZE; ++i) {
		if (is_valid_char(media_jack->eeprom_page0[SERIAL_NUM_OFFSET + i])) {
			serial_num_str[counter] = media_jack->eeprom_page0[SERIAL_NUM_OFFSET + i];
			counter++;
		}
	}
	serial_num_str[counter] = '\0';

	return 0;
}

#define DATE_CODE_OFFSET 182
static int sl_media_eeprom_date_code_get(struct sl_media_jack *media_jack, char *date_code_str)
{
	date_code_str[0] = media_jack->eeprom_page0[DATE_CODE_OFFSET];
	date_code_str[1] = media_jack->eeprom_page0[DATE_CODE_OFFSET + 1];
	date_code_str[2] = '-';
	date_code_str[3] = media_jack->eeprom_page0[DATE_CODE_OFFSET + 2];
	date_code_str[4] = media_jack->eeprom_page0[DATE_CODE_OFFSET + 3];
	date_code_str[5] = '-';
	date_code_str[6] = media_jack->eeprom_page0[DATE_CODE_OFFSET + 4];
	date_code_str[7] = media_jack->eeprom_page0[DATE_CODE_OFFSET + 5];
	date_code_str[8] = '\0';

	return 0;
}

#define FIRMWARE_VERSION_OFFSET 39
static int sl_media_eeprom_fw_ver_get(struct sl_media_jack *media_jack, u8 *fw_ver)
{
	fw_ver[0] = media_jack->eeprom_page0[FIRMWARE_VERSION_OFFSET];
	fw_ver[1] = media_jack->eeprom_page0[FIRMWARE_VERSION_OFFSET + 1];

	return 0;
}

#define CABLE_END_OFFSET 240
static int sl_media_eeprom_cable_end_get(struct sl_media_jack *media_jack)
{
	media_jack->cable_end = media_jack->eeprom_page0[CABLE_END_OFFSET];

	return 0;
}

#define CMIS_LENGTH_OFFSET    202
#define SFF8436_LENGTH_OFFSET 146
static int sl_media_eeprom_length_get(struct sl_media_jack *media_jack, u8 format, u32 *length)
{
	if (format == SL_MEDIA_MGMT_IF_CMIS) {
		*length = (media_jack->eeprom_page0[CMIS_LENGTH_OFFSET] & 0x3F);
		switch (media_jack->eeprom_page0[CMIS_LENGTH_OFFSET] & 0xC0) {
		case 0:
			*length *= 10;
			break;
		case 0x40:
			*length *= 100;
			break;
		case 0x80:
			*length *= 1000;
			break;
		case 0xC0:
			*length *= 10000;
			break;
		default:
			*length = 0;
			break;
		}
		return 0;
	}

	*length = media_jack->eeprom_page0[SFF8436_LENGTH_OFFSET] * 100;

	return 0;
}

/*
 * FIXME: Eventually remove this struct array and get this info from cable DB
 */
struct sl_media_fw_info {
	u32 type;
	u32 vendor;
	u8  fw_major_ver;
	u8  fw_minor_ver;
};

static struct sl_media_fw_info fw_db[] = {
		{
				.type         = SL_MEDIA_TYPE_AOC,
				.vendor       = SL_MEDIA_VENDOR_TE,
				.fw_major_ver = 0x04,
				.fw_minor_ver = 0x01,
		},
		{
				.type         = SL_MEDIA_TYPE_AEC,
				.vendor       = SL_MEDIA_VENDOR_TE,
				.fw_major_ver = 0x00,
				.fw_minor_ver = 0x04,
		},
		{
				.type         = SL_MEDIA_TYPE_AEC,
				.vendor       = SL_MEDIA_VENDOR_MOLEX,
				.fw_major_ver = 0x01,
				.fw_minor_ver = 0x01,
		},
		{
				.type         = SL_MEDIA_TYPE_AOC,
				.vendor       = SL_MEDIA_VENDOR_FINISAR,
				.fw_major_ver = 0x02,
				.fw_minor_ver = 0x06,
		},
};

static bool sl_media_is_fw_version_valid(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fw_db); ++i) {
		if ((fw_db[i].type == media_attr->type) &&
			(fw_db[i].vendor == media_attr->vendor) &&
			(fw_db[i].fw_major_ver == media_attr->fw_ver[0]) &&
			(fw_db[i].fw_minor_ver == media_attr->fw_ver[1])) {
			return true;
		}
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
		"not valid (vendor = %u, type = 0x%X, fw_major = 0x%X, fw_minor = 0x%X)",
		media_attr->vendor, media_attr->type, media_attr->fw_ver[0], media_attr->fw_ver[1]);

	return false;
}

void sl_media_eeprom_target_fw_ver_get(struct sl_media_jack *media_jack, char *target_fw_str, size_t target_fw_size)
{
	u32 type;
	u32 vendor;
	int i;

	type   = media_jack->cable_info[0].media_attr.type;
	vendor = media_jack->cable_info[0].media_attr.vendor;

	for (i = 0; i < ARRAY_SIZE(fw_db); ++i) {
		if (fw_db[i].type != type)
			continue;
		if (fw_db[i].vendor != vendor)
			continue;
		snprintf(target_fw_str, target_fw_size, "0x%u%u",
			 fw_db[i].fw_major_ver, fw_db[i].fw_minor_ver);
		return;
	}

	strncpy(target_fw_str, "none", target_fw_size - 1);
	target_fw_str[target_fw_size - 1] = '\0';
}

int sl_media_eeprom_parse(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	u8 format;

	sl_media_log_dbg(media_jack, LOG_NAME, "parse");

	if (sl_media_eeprom_format_get(media_jack, &format)                                           ||
		sl_media_eeprom_type_get(media_jack, format, &(media_attr->type))                     ||
		sl_media_eeprom_vendor_get(media_jack, format, &(media_attr->vendor))                 ||
		sl_media_eeprom_hpe_pn_get(media_jack, &(media_attr->hpe_pn), media_attr->hpe_pn_str) ||
		sl_media_eeprom_serial_num_get(media_jack, media_attr->serial_num_str)                ||
		sl_media_eeprom_date_code_get(media_jack, media_attr->date_code_str)                  ||
		sl_media_eeprom_fw_ver_get(media_jack, media_attr->fw_ver)                            ||
		sl_media_eeprom_cable_end_get(media_jack)                                             ||
		sl_media_eeprom_length_get(media_jack, format, &(media_attr->length_cm))              ||
		sl_media_eeprom_appsel_info_get(media_jack, &(media_attr->speeds_map))                ||
		sl_media_eeprom_furcation_get(media_jack, &(media_attr->furcation)))
		return -ENODATA;

	if ((media_attr->type == SL_MEDIA_TYPE_AOC) || (media_attr->type == SL_MEDIA_TYPE_AEC))
		if (!sl_media_is_fw_version_valid(media_jack, media_attr))
			media_attr->options |= SL_MEDIA_OPT_CABLE_FW_INVALID;

	if (media_attr->type == SL_MEDIA_TYPE_PEC)
		media_attr->options |= SL_MEDIA_OPT_AUTONEG;

	sl_media_log_dbg(media_jack, LOG_NAME,
		"parse (vendor = %u, type = 0x%X, length_cm = %u, speeds_map = 0x%X)",
		media_attr->vendor, media_attr->type, media_attr->length_cm, media_attr->speeds_map);
	sl_media_log_dbg(media_jack, LOG_NAME,
		"parse (hpe_pn = %u, serial_num = %s, options = 0x%X)",
		media_attr->hpe_pn, media_attr->serial_num_str, media_attr->options);

	return 0;
}

#define MEDIA_INTERFACE_CODE_OFFSET 87
u8 sl_media_eeprom_media_interface_get(struct sl_media_jack *media_jack)
{
	u8 media_interface;

	spin_lock(&media_jack->data_lock);
	media_interface = media_jack->eeprom_page0[MEDIA_INTERFACE_CODE_OFFSET];
	spin_unlock(&media_jack->data_lock);

	return media_interface;
}
