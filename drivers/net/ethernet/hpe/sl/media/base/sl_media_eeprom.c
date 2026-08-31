// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023-2026 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_media_io.h"
#include "sl_media_eeprom.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "base/sl_media_log.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_cable_db_load.h"

#define LOG_NAME SL_MEDIA_EEPROM_LOG_NAME

static void sl_media_eeprom_appsel_info_store(struct sl_media_jack *media_jack, u8 host_interface,
					      unsigned long *speeds_map, u8 appsel_num, u8 lane_count)
{
	switch (host_interface) {
	case SL_MEDIA_SS1_HOST_INTERFACE_50GAUI_1_C2M:
	case SL_MEDIA_SS1_HOST_INTERFACE_50GBASE_CR:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_100GAUI_2_C2M:
	case SL_MEDIA_SS1_HOST_INTERFACE_100GBASE_CR2:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_100G;
		break;
	case SL_MEDIA_SS2_HOST_INTERFACE_100GBASE_CR1:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		fallthrough;
	case SL_MEDIA_SS2_HOST_INTERFACE_100GAUI_1_L_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_100G;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_200GBASE_CR4:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_200GAUI_4_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
		media_jack->appsel_num_200_gaui = appsel_num;
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
		if (media_jack->appsel_num_400_gaui) /* we prefer 4_l_c2m over 4_s_c2m for default ck400G speed */
			break;
		media_jack->appsel_num_400_gaui = appsel_num;
		media_jack->lane_count_400_gaui = lane_count;
		media_jack->host_interface_400_gaui = host_interface;
		break;
	case SL_MEDIA_SS1_HOST_INTERFACE_400GAUI_4_L_C2M:
		*speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		media_jack->appsel_num_400_gaui = appsel_num;
		media_jack->lane_count_400_gaui = lane_count;
		media_jack->host_interface_400_gaui = host_interface;
		fallthrough;
	case SL_MEDIA_SS1_HOST_INTERFACE_400GBASE_CR8:
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
			media_jack->appsel_num_400_gaui = appsel_num;
			media_jack->host_interface_400_gaui = host_interface;
		}
		break;
	default:
		sl_media_log_dbg(media_jack, LOG_NAME,
				"invalid host interface (0x%x)", host_interface);
	}
}

#define SFF_EXTENDED_SPEC_COMPLIANCE_CODE_OFFSET 192
#define SFF_EXT_SPEC_COMP_256GFC_SW4             0x30
#define SFF_EXT_SPEC_COMP_64GFC_SW4              0x31
#define SFF_EXT_SPEC_COMP_200GBASE_SR4           0x32
#define SFF_EXT_SPEC_COMP_50GBASE_LR             0x33
#define SFF_EXT_SPEC_COMP_200GBASE_CR4           0x40
static int sl_media_eeprom_sff_appsel_info_get(struct sl_media_jack *media_jack, unsigned long *speeds_map)
{
	u8 ext_spec_comp;

	ext_spec_comp = media_jack->eeprom_page0[SFF_EXTENDED_SPEC_COMPLIANCE_CODE_OFFSET];

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "sff appsel info get (ext_spec_compliance = 0x%X)", ext_spec_comp);

	switch (ext_spec_comp) {
	case SFF_EXT_SPEC_COMP_200GBASE_CR4:
		*speeds_map = SL_MEDIA_SPEEDS_SUPPORT_BS_200G |
			      SL_MEDIA_SPEEDS_SUPPORT_CD_100G |
			      SL_MEDIA_SPEEDS_SUPPORT_CD_50G  |
			      SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		break;
	case SFF_EXT_SPEC_COMP_256GFC_SW4:
	case SFF_EXT_SPEC_COMP_64GFC_SW4:
	case SFF_EXT_SPEC_COMP_200GBASE_SR4:
	case SFF_EXT_SPEC_COMP_50GBASE_LR:
		*speeds_map = SL_MEDIA_SPEEDS_SUPPORT_BS_200G |
			      SL_MEDIA_SPEEDS_SUPPORT_CD_100G |
			      SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		break;
	default:
		*speeds_map = 0;
		break;
	}

	return 0;
}

#define APPSEL_PAGE0_START_OFFSET 86
#define APPSEL_PAGE1_START_OFFSET 223
#define APPSEL_STRIDE             4
#define APPSEL_END                0xFF
#define APPSEL_LAST_PAGE0         114
#define APPSEL_LAST_PAGE1         247
#define APPSEL_LANE_COUNT_OFFSET  2
static int sl_media_eeprom_appsel_info_get(struct sl_media_jack *media_jack, u8 format, unsigned long *speeds_map)
{
	u8  host_interface;
	u8  appsel_curr;
	u8  appsel_num;

	if (format != SL_MEDIA_MGMT_IF_CMIS)
		return sl_media_eeprom_sff_appsel_info_get(media_jack, speeds_map);

	appsel_curr = APPSEL_PAGE0_START_OFFSET;
	appsel_num = 1;
	while (appsel_curr <= APPSEL_LAST_PAGE0) {
		host_interface = media_jack->eeprom_page0[appsel_curr];
		if (host_interface == APPSEL_END)
			return 0;
		sl_media_eeprom_appsel_info_store(media_jack, host_interface, speeds_map, appsel_num,
						  media_jack->eeprom_page0[appsel_curr + APPSEL_LANE_COUNT_OFFSET]);
		appsel_curr += APPSEL_STRIDE;
		appsel_num++;
	}

	appsel_curr = APPSEL_PAGE1_START_OFFSET;
	while (appsel_curr <= APPSEL_LAST_PAGE1) {
		host_interface = media_jack->eeprom_page1[appsel_curr];
		if (host_interface == APPSEL_END)
			return 0;
		sl_media_eeprom_appsel_info_store(media_jack, host_interface, speeds_map, appsel_num,
						  media_jack->eeprom_page1[appsel_curr + APPSEL_LANE_COUNT_OFFSET]);
		appsel_curr += APPSEL_STRIDE;
		appsel_num++;
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

#define CMIS_CONNECTOR_TYPE_OFFSET    203
#define CMIS_CONNECTOR_POC_SR8        0x28
#define CMIS_CONNECTOR_POC_SR4        0x0C
#define SFF_CONNECTOR_TYPE_OFFSET     130
#define SFF_CONNECTOR_AOC             0x23
static bool sl_media_eeprom_is_type_poc(struct sl_media_jack *media_jack, u8 format)
{
	if (format == SL_MEDIA_MGMT_IF_CMIS) {
		switch (media_jack->eeprom_page0[CMIS_CONNECTOR_TYPE_OFFSET]) {
		case CMIS_CONNECTOR_POC_SR8:
		case CMIS_CONNECTOR_POC_SR4:
			return true;
		default:
			return false;
		}
	}

	/* SFF */
	switch (media_jack->eeprom_page0[SFF_CONNECTOR_TYPE_OFFSET]) {
	case SFF_CONNECTOR_AOC:
		return false;
	default:
		return true;
	}
}

#define CMIS_TYPE_OFFSET    212
#define SFF_TYPE_OFFSET     147
#define MEDIA_AOC           0x09
#define MEDIA_PEC_UNEQ      0x0A /* copper cable unequalized */
#define MEDIA_PEC_EQ        0x0B /* copper cable equalized */
#define MEDIA_AEC           0x0C /* copper cable, near and far end limiting active equalizers */
#define MEDIA_ACC           0x0F /* copper cable, linear active equalizers */
static int sl_media_eeprom_type_get(struct sl_media_jack *media_jack, u8 format, u32 vendor, u32 *type)
{
	u8 media;

	if (format == SL_MEDIA_MGMT_IF_CMIS)
		media = media_jack->eeprom_page0[CMIS_TYPE_OFFSET] & 0x0F;
	else
		media = media_jack->eeprom_page0[SFF_TYPE_OFFSET] >> 4;

	if (media <= MEDIA_AOC)
		*type = sl_media_eeprom_is_type_poc(media_jack, format) ? SL_MEDIA_TYPE_POC : SL_MEDIA_TYPE_AOC;
	else if (media == MEDIA_PEC_UNEQ || media == MEDIA_PEC_EQ)
		*type = SL_MEDIA_TYPE_PEC;
	else if (media == MEDIA_AEC)
		*type = SL_MEDIA_TYPE_AEC;
	else if (media == MEDIA_ACC) {
		/* compensate for Molex type programming issue */
		if (vendor == SL_MEDIA_VENDOR_MOLEX)
			*type = SL_MEDIA_TYPE_AEC;
		else
			*type = SL_MEDIA_TYPE_ACC;
	} else {
		*type = SL_MEDIA_TYPE_UNSUPPORTED;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "type get (media = 0x%X, type = 0x%X %s)",
			 media, *type, sl_media_type_str(*type));

	return 0;
}

#define CMIS_VENDOR_OFFSET      129
#define SFF_VENDOR_OFFSET       148
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
	{ .name = "Leoni",
	  .type = SL_MEDIA_VENDOR_LEONI },
};
static int sl_media_eeprom_vendor_get(struct sl_media_jack *media_jack, u8 format, u32 *vendor)
{
	const char *vendor_ptr;
	int         x;

	if (format == SL_MEDIA_MGMT_IF_CMIS)
		vendor_ptr = &(media_jack->eeprom_page0[CMIS_VENDOR_OFFSET]);
	else
		vendor_ptr = &(media_jack->eeprom_page0[SFF_VENDOR_OFFSET]);

	for (x = 0; x < ARRAY_SIZE(vendor_list); ++x) {
		if (strnstr(vendor_ptr, vendor_list[x].name, VENDOR_NAME_MAX_LEN) == NULL)
			continue;
		*vendor = vendor_list[x].type;
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "vendor get (vendor = %u %s)", *vendor, sl_media_vendor_str(*vendor));
		return 0;
	}

	*vendor = 0;

	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_EEPROM_VENDOR_UNSUPPORTED);
	sl_media_log_err_trace(media_jack, LOG_NAME, "eeprom vendor get unsupported");

	return 0;
}

static bool is_valid_char(char c)
{
	return c >= 33 && c <= 126;
}

#define CMIS_VENDOR_PN_OFFSET 148
#define SFF_VENDOR_PN_OFFSET  168
static int sl_media_eeprom_vendor_pn_str_get(struct sl_media_jack *media_jack, u8 format, char *vendor_pn_str)
{
	int x;
	int pos;
	u8  vendor_pn_offset;

	if (format == SL_MEDIA_MGMT_IF_CMIS)
		vendor_pn_offset = CMIS_VENDOR_PN_OFFSET;
	else
		vendor_pn_offset = SFF_VENDOR_PN_OFFSET;

	pos = 0;
	memset(vendor_pn_str, 0, SL_MEDIA_VENDOR_PN_SIZE);
	for (x = 0; x < SL_MEDIA_VENDOR_PN_SIZE - 1; ++x) {
		if (is_valid_char(media_jack->eeprom_page0[vendor_pn_offset + x])) {
			vendor_pn_str[pos] = media_jack->eeprom_page0[vendor_pn_offset + x];
			pos++;
		}
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "vendor part num get (par_num = %s)", vendor_pn_str);

	return 0;
}

#define COMPANY_NAME_OFFSET 224
#define HPE_PN_OFFSET       228
#define CRAY_PN_OFFSET      229
static int sl_media_eeprom_hpe_pn_get(struct sl_media_jack *media_jack, u8 format, u32 *hpe_pn, char *hpe_pn_str)
{
	const char *company;
	const char *pn_ptr;
	int         pn_offset;
	int         pn_size;
	int         i;
	int         counter;

	company = &media_jack->eeprom_page0[COMPANY_NAME_OFFSET];
	if (strncasecmp(company, "CRAY", 4) == 0) {
		pn_offset = CRAY_PN_OFFSET;
		pn_size   = SL_MEDIA_CRAY_PN_SIZE - 1;
	} else if (strncasecmp(company, "HPE", 3) == 0) {
		pn_offset = HPE_PN_OFFSET;
		pn_size   = SL_MEDIA_HPE_PN_SIZE - 1;
	} else {
		hpe_pn_str[0] = '\0';
		*hpe_pn = 0;
		return 0;
	}

	pn_ptr  = &media_jack->eeprom_page0[pn_offset];
	counter = 0;
	for (i = 0; i < pn_size; ++i) {
		if (is_valid_char(pn_ptr[i]))
			hpe_pn_str[counter++] = pn_ptr[i];
	}
	hpe_pn_str[counter] = '\0';

	/* build numeric value by accumulating digit characters only */
	*hpe_pn = 0;
	for (i = 0; i < pn_size; ++i) {
		if (pn_ptr[i] < '0' || pn_ptr[i] > '9') /* if the PN character is not numeric, ignore it */
			continue;
		*hpe_pn = *hpe_pn * 10 + (pn_ptr[i] - '0');
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "hpe part num get (hpe_pn = %s)", hpe_pn_str);

	return 0;
}

#define CMIS_SERIAL_NUM_OFFSET 166
#define SFF_SERIAL_NUM_OFFSET  196
static int sl_media_eeprom_serial_num_get(struct sl_media_jack *media_jack, u8 format, char *serial_num_str)
{
	int i;
	int counter;
	u8  serial_num_offset;

	if (format == SL_MEDIA_MGMT_IF_CMIS)
		serial_num_offset = CMIS_SERIAL_NUM_OFFSET;
	else
		serial_num_offset = SFF_SERIAL_NUM_OFFSET;

	counter = 0;
	memset(serial_num_str, '\0', SL_MEDIA_SERIAL_NUM_SIZE);
	for (i = 0; i < (SL_MEDIA_SERIAL_NUM_SIZE - 1); ++i) {
		if (media_jack->eeprom_page0[serial_num_offset + i] == '-')
			break;

		if (is_valid_char(media_jack->eeprom_page0[serial_num_offset + i])) {
			serial_num_str[counter] = media_jack->eeprom_page0[serial_num_offset + i];
			counter++;
		}
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "serial num get (serial_num = %s)", serial_num_str);

	return 0;
}

#define CMIS_DATE_CODE_OFFSET 182
#define SFF_DATE_CODE_OFFSET  212
static int sl_media_eeprom_date_code_get(struct sl_media_jack *media_jack, u8 format, char *date_code_str)
{
	u8 date_code_offset;

	if (format == SL_MEDIA_MGMT_IF_CMIS)
		date_code_offset = CMIS_DATE_CODE_OFFSET;
	else
		date_code_offset = SFF_DATE_CODE_OFFSET;

	date_code_str[0] = media_jack->eeprom_page0[date_code_offset];
	date_code_str[1] = media_jack->eeprom_page0[date_code_offset + 1];
	date_code_str[2] = '-';
	date_code_str[3] = media_jack->eeprom_page0[date_code_offset + 2];
	date_code_str[4] = media_jack->eeprom_page0[date_code_offset + 3];
	date_code_str[5] = '-';
	date_code_str[6] = media_jack->eeprom_page0[date_code_offset + 4];
	date_code_str[7] = media_jack->eeprom_page0[date_code_offset + 5];
	date_code_str[8] = '\0';

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "date code get (date = %s)", date_code_str);

	return 0;
}

#define FIRMWARE_VERSION_OFFSET 39
static int sl_media_eeprom_fw_ver_get(struct sl_media_jack *media_jack, u8 format, u8 *fw_ver)
{
	if (format == SL_MEDIA_MGMT_IF_CMIS) {
		fw_ver[0] = media_jack->eeprom_page0[FIRMWARE_VERSION_OFFSET];
		fw_ver[1] = media_jack->eeprom_page0[FIRMWARE_VERSION_OFFSET + 1];
	} else {
		fw_ver[0] = 0;
		fw_ver[1] = 0;
	}

	return 0;
}

#define CABLE_END_OFFSET 240
static int sl_media_eeprom_cable_end_get(struct sl_media_jack *media_jack)
{
	media_jack->cable_end = media_jack->eeprom_page0[CABLE_END_OFFSET];

	return 0;
}

#define CMIS_LENGTH_OFFSET  202
#define SFF_LENGTH_OFFSET   146
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
	} else {
		*length = media_jack->eeprom_page0[SFF_LENGTH_OFFSET] * 100;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "length get (length = %ucm)", *length);

	return 0;
}

#define SUPPORTED_FLAGS_ADVERTISEMENT_OFFSET 157
static void sl_media_eeprom_supported_flags_advertised_get(struct sl_media_jack *media_jack,
							   u8 *supported_flags_advertised)
{
	spin_lock(&media_jack->data_lock);
	memcpy(supported_flags_advertised, &media_jack->eeprom_page1[SUPPORTED_FLAGS_ADVERTISEMENT_OFFSET],
	       SL_MEDIA_SUPPORTED_FLAGS_ADVERTISED_SIZE);
	spin_unlock(&media_jack->data_lock);
}

bool sl_media_eeprom_is_fw_version_supported(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	struct sl_media_cable_attr entry;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "is fw version supported (type = 0x%X %s, shape = %u %s, end = %u %s, idx = %d)",
			 media_attr->type, sl_media_type_str(media_attr->type),
			 media_attr->shape, sl_media_shape_str(media_attr->shape),
			 media_jack->cable_end, sl_media_cable_end_str(media_jack->cable_end),
			 media_jack->cable_db_idx);

	sl_media_data_cable_db_entry_get(media_jack, &entry);

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "is fw version supported (fw_ver = %02X.%02X, db_fw_ver = %02X.%02X, db_split_fw_ver = %02X.%02X)",
			 media_attr->fw_ver[0], media_attr->fw_ver[1],
			 entry.fw_ver.major, entry.fw_ver.minor,
			 entry.fw_ver.split_major, entry.fw_ver.split_minor);

	if (media_attr->shape == SL_MEDIA_SHAPE_SPLITTER && media_jack->cable_end != SL_MEDIA_CABLE_END_DD)
		return ((media_attr->fw_ver[0] > entry.fw_ver.split_major) ||
			((media_attr->fw_ver[0] == entry.fw_ver.split_major) &&
			(media_attr->fw_ver[1] >= entry.fw_ver.split_minor)));

	return ((media_attr->fw_ver[0] > entry.fw_ver.major) ||
		((media_attr->fw_ver[0] == entry.fw_ver.major) &&
		(media_attr->fw_ver[1] >= entry.fw_ver.minor)));
}

int sl_media_eeprom_target_fw_ver_str_get(struct sl_media_jack *media_jack, char *target_fw_str, size_t target_fw_size)
{
	struct sl_media_cable_attr entry;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "target fw ver get (idx = %u, type = 0x%X %s, shape = %u %s, end = %u %s, supported = %s)",
			 media_jack->cable_db_idx,
			 media_jack->cable_info[0].media_attr.type,
			 sl_media_type_str(media_jack->cable_info[0].media_attr.type),
			 media_jack->cable_info[0].media_attr.shape,
			 sl_media_shape_str(media_jack->cable_info[0].media_attr.shape),
			 media_jack->cable_end,
			 sl_media_cable_end_str(media_jack->cable_end),
			 media_jack->is_cable_unsupported ? "no" : "yes");

	if (!SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_jack->cable_info[0].media_attr.type)) {
		snprintf(target_fw_str, target_fw_size, "invalid-type\n");
		return 0;
	}

	if (media_jack->is_cable_unsupported) {
		snprintf(target_fw_str, target_fw_size, "unsupported\n");
		return 0;
	}

	if (media_jack->is_supported_ss200_cable) {
		snprintf(target_fw_str, target_fw_size, "ss200-cable\n");
		return 0;
	}

	sl_media_data_cable_db_entry_get(media_jack, &entry);

	if (media_jack->cable_info[0].media_attr.shape == SL_MEDIA_SHAPE_SPLITTER &&
	    media_jack->cable_end != SL_MEDIA_CABLE_END_DD) {
		snprintf(target_fw_str, target_fw_size, "%02X.%02X\n",
			 entry.fw_ver.split_major, entry.fw_ver.split_minor);
		return 0;
	}

	snprintf(target_fw_str, target_fw_size, "%02X.%02X\n", entry.fw_ver.major, entry.fw_ver.minor);

	return 0;
}

#define LOOPBACK_CAPABILITIES_OFFSET 128
static void sl_media_eeprom_loopback_caps_get(struct sl_media_jack *media_jack, u32 type, u8 *loopback_caps)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "loopback caps get (type = 0x%X %s)", type, sl_media_type_str(type));

	if (!SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(type)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "loopback caps get (type = 0x%X %s)",
				 type, sl_media_type_str(type));
		*loopback_caps = 0;
		return;
	}

	rtn = sl_media_io_read8(media_jack, 0x13, LOOPBACK_CAPABILITIES_OFFSET, loopback_caps);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "media_io_read8 failed [%d]", rtn);
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_EEPROM_JACK_IO);
		*loopback_caps = 0;
		return;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "get (loopback_caps = 0x%X)", *loopback_caps);
}

void sl_media_eeprom_parse(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "parse");

	/* vendor get MUST be first */
	sl_media_eeprom_vendor_get(media_jack, media_attr->format, &(media_attr->vendor));
	sl_media_eeprom_vendor_pn_str_get(media_jack, media_attr->format, media_attr->vendor_pn_str);
	sl_media_eeprom_type_get(media_jack, media_attr->format, media_attr->vendor, &(media_attr->type));
	sl_media_eeprom_hpe_pn_get(media_jack, media_attr->format, &(media_attr->hpe_pn), media_attr->hpe_pn_str);
	sl_media_eeprom_serial_num_get(media_jack, media_attr->format, media_attr->serial_num_str);
	sl_media_eeprom_date_code_get(media_jack, media_attr->format, media_attr->date_code_str);
	sl_media_eeprom_fw_ver_get(media_jack, media_attr->format, media_attr->fw_ver);
	sl_media_eeprom_cable_end_get(media_jack);
	sl_media_eeprom_length_get(media_jack, media_attr->format, &(media_attr->length_cm));
	sl_media_eeprom_appsel_info_get(media_jack, media_attr->format, &(media_attr->speeds_map));
	sl_media_eeprom_furcation_get(media_jack, &(media_attr->furcation));
	sl_media_eeprom_supported_flags_advertised_get(media_jack, media_attr->supported_flags_advertised);
	sl_media_eeprom_loopback_caps_get(media_jack, media_attr->type, &media_attr->loopback_caps);

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "parse (format = %u, vendor = %u %s, vendor_pn_str = %s, fw = %02X.%02X)",
			 media_attr->format, media_attr->vendor, sl_media_vendor_str(media_attr->vendor),
			 media_attr->vendor_pn_str, media_attr->fw_ver[0], media_attr->fw_ver[1]);
	sl_media_log_dbg(media_jack, LOG_NAME,
			 "parse (type = 0x%X %s, length = %ucm, speeds_map = 0x%lX)",
			 media_attr->type, sl_media_type_str(media_attr->type), media_attr->length_cm,
			 media_attr->speeds_map);
	sl_media_log_dbg(media_jack, LOG_NAME,
			 "parse (hpe_pn = %u, serial_num = %s, errors = 0x%X, info = 0x%X)",
			 media_attr->hpe_pn, media_attr->serial_num_str, media_attr->errors, media_attr->info);
}

#define IDENTIFIER_OFFSET 0
#define REV_CMPL_OFFSET   1
int sl_media_eeprom_format_get(struct sl_media_jack *media_jack, u8 *format, u8 *version)
{
	u8   identifier;
	u8   revision;
	bool is_sff;
	bool is_cmis;

	sl_media_log_dbg(media_jack, LOG_NAME, "format get");

	identifier = media_jack->eeprom_page0[IDENTIFIER_OFFSET];
	revision   = media_jack->eeprom_page0[REV_CMPL_OFFSET];

	is_sff     = (identifier == 0x0D || identifier == 0x11) && (revision < 0x27);
	is_cmis    = ((identifier == 0x18) || (identifier == 0x19) || (identifier >= 0x1E && identifier <= 0x25)) &&
		     ((revision & 0xF0) >= 0x30 && (revision & 0xF0) <= 0x50);

	sl_media_log_dbg(media_jack, LOG_NAME, "format get (id = 0x%X, rev = 0x%X)", identifier, revision);

	if (is_sff) {
		*version = 0;
		*format  = SL_MEDIA_MGMT_IF_SFF8636;
		sl_media_log_dbg(media_jack, LOG_NAME, "format SFF (id = 0x%X, rev = 0x%X)", identifier, revision);
		return 0;
	}

	if (is_cmis) {
		*version = ((revision & 0xF0) >> 4);
		*format  = SL_MEDIA_MGMT_IF_CMIS;
		sl_media_log_dbg(media_jack, LOG_NAME, "format CMIS (id = 0x%X, rev = 0x%X)", identifier, revision);
		return 0;
	}

	*version = 0;
	*format  = 0;

	return -EMEDIUMTYPE;
}

#define MEDIA_INTERFACE_CODE_OFFSET 87
int sl_media_eeprom_media_interface_get(struct sl_media_jack *media_jack, u8 *media_interface)
{
	spin_lock(&media_jack->data_lock);
	*media_interface = media_jack->eeprom_page0[MEDIA_INTERFACE_CODE_OFFSET];
	spin_unlock(&media_jack->data_lock);

	return 0;
}

bool sl_media_eeprom_is_cmis(struct sl_media_jack *media_jack)
{
	u8 format;
	u8 version;

	sl_media_eeprom_format_get(media_jack, &format, &version);
	return (format == SL_MEDIA_MGMT_IF_CMIS);
}
