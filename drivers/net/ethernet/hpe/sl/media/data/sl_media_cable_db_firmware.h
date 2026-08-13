/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_CABLE_DB_FIRMWARE_H_
#define _SL_MEDIA_CABLE_DB_FIRMWARE_H_

#include <linux/types.h>
#include <linux/build_bug.h>

#include <linux/hpe/sl/sl_media.h>

#define SL_MEDIA_CABLE_DB_FW_FILE        "sl_cable_db.bin"

#define SL_MEDIA_CABLE_DB_MAGIC          0x44434C53U   /* 'S','L','C','D' */
#define SL_MEDIA_CABLE_DB_MAGIC_OFFSET   0
#define SL_MEDIA_CABLE_DB_HDR_VER_OFFSET 4

#define SL_MEDIA_CABLE_DB_HDR_VERSION    1U
#define SL_MEDIA_CABLE_DB_DATA_VERSION   1U

// FIXME: need to add this info to sysfs
struct sl_media_cable_db_hdr_v1 {
	u32 magic;
	u32 hdr_version;
	u32 data_version;
	u32 count;
	u32 date_s;
};

#define SL_MEDIA_DB_HDR_SIZE sizeof(struct sl_media_cable_db_hdr_v1)

static_assert(sizeof(struct sl_media_cable_db_hdr_v1) == SL_MEDIA_DB_HDR_SIZE,
	      "sl_media_cable_db_hdr_v1 size changed - update binary format version");

#define SL_MEDIA_CABLE_DB_RECORD_BASE_SIZE_BYTES 64
union sl_media_cable_db_record_v1 {
	u8 bytes[SL_MEDIA_CABLE_DB_RECORD_BASE_SIZE_BYTES];
	struct {
		u32 hpe_pn;
		u32 vendor;
		u32 type;
		u32 shape;
		u32 length_cm;
		u32 max_speed;
		s16 serdes_pre1;
		s16 serdes_pre2;
		s16 serdes_pre3;
		s16 serdes_cursor;
		s16 serdes_post1;
		s16 serdes_post2;
		s8  fw_ver_major;
		s8  fw_ver_minor;
		s8  fw_ver_split_major;
		s8  fw_ver_split_minor;
		u8  is_supported_ss200_cable;
		u8  vendor_pn_str[SL_MEDIA_VENDOR_PN_SIZE];
	};
};

#define SL_MEDIA_CABLE_DB_RECORD_V1_SIZE SL_MEDIA_CABLE_DB_RECORD_BASE_SIZE_BYTES

static_assert(sizeof(union sl_media_cable_db_record_v1) == SL_MEDIA_CABLE_DB_RECORD_BASE_SIZE_BYTES,
	      "sl_media_cable_db_record_v1 size changed - update binary format version");

#endif /* _SL_MEDIA_CABLE_DB_FIRMWARE_H_ */
