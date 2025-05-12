/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_MEDIA_H_
#define _LINUX_SL_MEDIA_H_

#include <linux/bitops.h>

struct sl_lgrp;

#define SL_MEDIA_FURCATION_INVALID 0
#define SL_MEDIA_FURCATION_X1      0x1 /* Unfurcated   */
#define SL_MEDIA_FURCATION_X2      0x3 /* Bifurcated   */
#define SL_MEDIA_FURCATION_X4      0xF /* Quadfurcated */

#define SL_MEDIA_TYPE_HEADSHELL     BIT(0)
#define SL_MEDIA_TYPE_BACKPLANE     BIT(1)
#define SL_MEDIA_TYPE_ELECTRICAL    BIT(8)
#define SL_MEDIA_TYPE_OPTICAL       BIT(9)
#define SL_MEDIA_TYPE_PASSIVE       BIT(16)
#define SL_MEDIA_TYPE_ACTIVE        BIT(17)
#define SL_MEDIA_TYPE_ANALOG        BIT(24)
#define SL_MEDIA_TYPE_DIGITAL       BIT(25)
#define SL_MEDIA_SIGNAL_RETIME      BIT(26)
#define SL_MEDIA_SIGNAL_REDRIVE     BIT(27)
#define SL_MEDIA_TYPE_LOOPBACK      BIT(28)
#define SL_MEDIA_TYPE_NOT_SUPPORTED BIT(29)

#define SL_MEDIA_TYPE_INVALID		0
#define SL_MEDIA_TYPE_AOC		(SL_MEDIA_TYPE_HEADSHELL  | \
					 SL_MEDIA_TYPE_OPTICAL    | \
					 SL_MEDIA_TYPE_ACTIVE)
/* PEC is same type as DAC */
#define SL_MEDIA_TYPE_PEC		(SL_MEDIA_TYPE_HEADSHELL  | \
					 SL_MEDIA_TYPE_ELECTRICAL | \
					 SL_MEDIA_TYPE_PASSIVE)
#define SL_MEDIA_TYPE_AEC		(SL_MEDIA_TYPE_HEADSHELL  | \
					 SL_MEDIA_TYPE_ELECTRICAL | \
					 SL_MEDIA_TYPE_ACTIVE     | \
					 SL_MEDIA_SIGNAL_RETIME)
#define SL_MEDIA_TYPE_ACC		(SL_MEDIA_TYPE_HEADSHELL  | \
					 SL_MEDIA_TYPE_ELECTRICAL | \
					 SL_MEDIA_TYPE_ACTIVE     | \
					 SL_MEDIA_SIGNAL_REDRIVE)
#define SL_MEDIA_TYPE_BKP		(SL_MEDIA_TYPE_BACKPLANE  | \
					 SL_MEDIA_TYPE_ELECTRICAL | \
					 SL_MEDIA_TYPE_PASSIVE)

enum sl_media_vendor {
	SL_MEDIA_VENDOR_INVALID   = 0,
	SL_MEDIA_VENDOR_UNKNOWN,

	SL_MEDIA_VENDOR_TE,
	SL_MEDIA_VENDOR_HISENSE,
	SL_MEDIA_VENDOR_LEONI,
	SL_MEDIA_VENDOR_FINISAR,
	SL_MEDIA_VENDOR_MOLEX,
	SL_MEDIA_VENDOR_BIZLINK,
	SL_MEDIA_VENDOR_HPE,
	SL_MEDIA_VENDOR_CLOUD_LIGHT,
	SL_MEDIA_VENDOR_MULTILANE,
	SL_MEDIA_VENDOR_AMPHENOL,
};

enum sl_media_shape {
	SL_MEDIA_SHAPE_INVALID = 0,
	SL_MEDIA_SHAPE_STRAIGHT,
	SL_MEDIA_SHAPE_SPLITTER,
	SL_MEDIA_SHAPE_BIFURCATED,
};

#define SL_MEDIA_SPEEDS_SUPPORT_CK_400G BIT(0)
#define SL_MEDIA_SPEEDS_SUPPORT_CK_200G BIT(1)
#define SL_MEDIA_SPEEDS_SUPPORT_CK_100G BIT(2)
#define SL_MEDIA_SPEEDS_SUPPORT_BS_200G BIT(3)
#define SL_MEDIA_SPEEDS_SUPPORT_BJ_100G BIT(4)
#define SL_MEDIA_SPEEDS_SUPPORT_CD_100G BIT(5)
#define SL_MEDIA_SPEEDS_SUPPORT_CD_50G  BIT(6)
#define SL_MEDIA_SPEEDS_SUPPORT_CK_800G BIT(7)
#define SL_MEDIA_SPEEDS_SUPPORT_INVALID BIT(8)

#define SL_MEDIA_OPT_AUTONEG              BIT(0)
#define SL_MEDIA_OPT_FAKE                 BIT(1)
#define SL_MEDIA_OPT_RESETTABLE           BIT(2)
#define SL_MEDIA_OPT_LINKTRAIN            BIT(3)
#define SL_MEDIA_OPT_CABLE_NOT_SUPPORTED  BIT(4)
#define SL_MEDIA_OPT_CABLE_FORMAT_INVALID BIT(5)
#define SL_MEDIA_OPT_SS200_CABLE          BIT(6)

#define SL_MEDIA_QSFP_DENSITY_INVALID  0
#define SL_MEDIA_QSFP_DENSITY_SINGLE   1
#define SL_MEDIA_QSFP_DENSITY_DOUBLE   2
#define SL_MEDIA_QSFP_DENSITY_EXTENDED 3

#define SL_MEDIA_JACK_TYPE_INVALID   0
#define SL_MEDIA_JACK_TYPE_BACKPLANE 1
#define SL_MEDIA_JACK_TYPE_SFP       2
#define SL_MEDIA_JACK_TYPE_QSFP      3
#define SL_MEDIA_JACK_TYPE_OSFP      4


enum sl_media_mgmt_if {
	SL_MEDIA_MGMT_IF_UNKNOWN = 0,
	SL_MEDIA_MGMT_IF_SFF8636,
	SL_MEDIA_MGMT_IF_CMIS,
};

struct sl_media_qsfp {
	u32 density;
};

#define SL_MEDIA_SERIAL_NUM_SIZE       17
#define SL_MEDIA_HPE_PN_SIZE           13
#define SL_MEDIA_DATE_CODE_SIZE        9
#define SL_MEDIA_FIRMWARE_VERSION_SIZE 2

#define SL_MEDIA_ATTR_MAGIC 0x6c6d6d61
#define SL_MEDIA_ATTR_VER   9
struct sl_media_attr {
	u32 magic;
	u32 ver;
	u32 size;

	u32 type;
	u32 vendor;
	u32 length_cm;
	u32 speeds_map;                /* Supported speeds on cable */
	u32 hpe_pn;                    /* HPE part number */
	u32 shape;
	char  hpe_pn_str[SL_MEDIA_HPE_PN_SIZE];
	char  serial_num_str[SL_MEDIA_SERIAL_NUM_SIZE];
	char  date_code_str[SL_MEDIA_DATE_CODE_SIZE];
	u8  fw_ver[SL_MEDIA_FIRMWARE_VERSION_SIZE];
	u32 max_speed;

	u32 furcation;

	u32 jack_type;
	union {
		struct sl_media_qsfp qsfp;
	} jack_type_info;

	u32 options;
};

struct sl_lgrp;

int sl_media_cable_insert(struct sl_lgrp *lgrp, u8 *eeprom_page0,
			  u8 *eeprom_page1, u32 flags);
int sl_media_cable_remove(struct sl_lgrp *lgrp);

int sl_media_fake_headshell_insert(struct sl_lgrp *lgrp,  struct sl_media_attr *media_attr);
int sl_media_fake_headshell_remove(struct sl_lgrp *lgrp);

int sl_media_headshell_temp_get(struct sl_lgrp *lgrp, u8 *temp);

const char *sl_media_furcation_str(u32 furcation);
const char *sl_media_state_str(u8 state);
const char *sl_media_cable_shift_state_str(u8 cable_shift_state);
const char *sl_media_type_str(u32 type);
const char *sl_media_shape_str(u32 shape);
const char *sl_media_cable_end_str(u8 cable_end);
const char *sl_media_vendor_str(u32 vendor);
const char *sl_media_speed_str(u32 speed);
const char *sl_media_ber_str(u8 media_interface);
const char *sl_media_host_interface_str(u32 speed, u32 type);
const char *sl_media_opt_str(u32 option);
const char *sl_media_jack_type_str(u32 jack_type, u32 density);

#endif /* _LINUX_SL_MEDIA_H_ */
