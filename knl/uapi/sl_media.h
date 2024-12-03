/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021,2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _UAPI_SL_MEDIA_H_
#define _UAPI_SL_MEDIA_H_

#ifdef __KERNEL__
#include <linux/bitops.h>
#else
#ifndef BIT
#define BIT(_num)      (1UL << (_num))
#endif
#ifndef BIT_ULL
#define BIT_ULL(_num)  (1ULL << (_num))
#endif
#endif

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

#define SL_MEDIA_TYPE_INVALID   	0
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
};

#define SL_MEDIA_SPEEDS_SUPPORT_CK_400G BIT(0)
#define SL_MEDIA_SPEEDS_SUPPORT_CK_200G BIT(1)
#define SL_MEDIA_SPEEDS_SUPPORT_CK_100G BIT(2)
#define SL_MEDIA_SPEEDS_SUPPORT_BS_200G BIT(3)
#define SL_MEDIA_SPEEDS_SUPPORT_BJ_100G BIT(4)
#define SL_MEDIA_SPEEDS_SUPPORT_CD_100G BIT(5)
#define SL_MEDIA_SPEEDS_SUPPORT_CD_50G  BIT(6)
#define SL_MEDIA_SPEEDS_SUPPORT_CK_800G BIT(7)

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
	__u32 density;
};

#define SL_MEDIA_SERIAL_NUM_SIZE       17
#define SL_MEDIA_HPE_PN_SIZE           13
#define SL_MEDIA_DATE_CODE_SIZE        9
#define SL_MEDIA_FIRMWARE_VERSION_SIZE 2

#define SL_MEDIA_ATTR_MAGIC 0x6c6d6d61
#define SL_MEDIA_ATTR_VER   8
struct sl_media_attr {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 type;
	__u32 vendor;
	__u32 length_cm;
	__u32 speeds_map;                /* Supported speeds on cable */
	__u32 hpe_pn;                    /* HPE part number */
	char  hpe_pn_str[SL_MEDIA_HPE_PN_SIZE];
	char  serial_num_str[SL_MEDIA_SERIAL_NUM_SIZE];
	char  date_code_str[SL_MEDIA_DATE_CODE_SIZE];
	__u8  fw_ver[SL_MEDIA_FIRMWARE_VERSION_SIZE];
	__u32 max_speed;

	__u32 furcation;

	__u32 jack_type;
	union {
		struct sl_media_qsfp qsfp;
	} jack_type_info;

	__u32 options;
};

#endif /* _UAPI_SL_MEDIA_H_ */
