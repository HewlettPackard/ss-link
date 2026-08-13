// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_asic.h"
#include "sl_module.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "data/sl_media_cable_db_firmware.h"
#include "data/sl_media_cable_db_load.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_CABLE_LOG_NAME

static void sl_media_cable_db_record_to_attr(const void *bytes, u32 data_version,
					     struct sl_media_cable_attr *attr)
{
	const union sl_media_cable_db_record_v1 *v1;

	switch (data_version) {
	case SL_MEDIA_CABLE_DB_DATA_VERSION:
		v1 = bytes;

		attr->hpe_pn    = v1->hpe_pn;
		attr->vendor    = v1->vendor;
		attr->type      = v1->type;
		attr->shape     = v1->shape;
		attr->length_cm = v1->length_cm;
		attr->max_speed = v1->max_speed;

		attr->serdes_settings.pre1   = v1->serdes_pre1;
		attr->serdes_settings.pre2   = v1->serdes_pre2;
		attr->serdes_settings.pre3   = v1->serdes_pre3;
		attr->serdes_settings.cursor = v1->serdes_cursor;
		attr->serdes_settings.post1  = v1->serdes_post1;
		attr->serdes_settings.post2  = v1->serdes_post2;

		attr->fw_ver.major       = v1->fw_ver_major;
		attr->fw_ver.minor       = v1->fw_ver_minor;
		attr->fw_ver.split_major = v1->fw_ver_split_major;
		attr->fw_ver.split_minor = v1->fw_ver_split_minor;

		attr->is_supported_ss200_cable = v1->is_supported_ss200_cable;

		memcpy(attr->vendor_pn_str, v1->vendor_pn_str, sizeof(attr->vendor_pn_str));
		attr->vendor_pn_str[SL_MEDIA_VENDOR_PN_SIZE - 1] = '\0';

		break;
	}
}

void sl_media_data_cable_db_entry_get_by_idx(const struct sl_media_ldev *media_ldev,
					     u32 idx, struct sl_media_cable_attr *attr)
{
	sl_media_cable_db_record_to_attr((const u8 *)media_ldev->cable_db.data + idx * media_ldev->cable_db.record_size,
					 media_ldev->cable_db.data_version, attr);
}

void sl_media_data_cable_db_entry_get(const struct sl_media_jack *media_jack,
				      struct sl_media_cable_attr *attr)
{
	sl_media_data_cable_db_entry_get_by_idx(media_jack->media_ldev,
						media_jack->cable_db_idx, attr);
}

int sl_media_data_cable_db_load(struct sl_media_ldev *media_ldev)
{
	const struct firmware           *fw;
	u32                              magic;
	u32                              hdr_version;
	u32                              data_version;
	u32                              count;
	u32                              hdr_size;
	u32                              record_size;
	u32                              expected_size;
	void                            *blob;
	int                              rtn;
	struct sl_media_cable_db_hdr_v1  hdr_v1;

	sl_media_log_dbg(media_ldev, LOG_NAME, "cable db load");

	rtn = request_firmware(&fw, SL_MEDIA_CABLE_DB_FW_FILE, sl_device_get());
	if (rtn) {
		sl_media_log_err(media_ldev, LOG_NAME,
				 "cable db load request_firmware failed [%d]", rtn);
		return -EIO;
	}

	memcpy(&magic, fw->data + SL_MEDIA_CABLE_DB_MAGIC_OFFSET, sizeof(magic));

	if (magic != SL_MEDIA_CABLE_DB_MAGIC) {
		sl_media_log_err(media_ldev, LOG_NAME,
				 "cable db load bad magic (actual = 0x%08X, expected = 0x%08X)",
				 magic, SL_MEDIA_CABLE_DB_MAGIC);
		rtn = -EINVAL;
		goto out_release;
	}

	memcpy(&hdr_version, fw->data + SL_MEDIA_CABLE_DB_HDR_VER_OFFSET, sizeof(hdr_version));

	sl_media_log_dbg(media_ldev, LOG_NAME, "cable db load (hdr_version = %u)", hdr_version);

	switch (hdr_version) {
	case SL_MEDIA_CABLE_DB_HDR_VERSION:
		memcpy(&hdr_v1, fw->data, sizeof(hdr_v1));
		data_version = hdr_v1.data_version;
		count        = hdr_v1.count;
		hdr_size     = sizeof(hdr_v1);
		break;
	default:
		sl_media_log_err(media_ldev, LOG_NAME,
				 "cable db load unsupported (hdr_version = %u)", hdr_version);
		rtn = -EINVAL;
		goto out_release;
	}

	sl_media_log_dbg(media_ldev, LOG_NAME,
			 "cable db load (data_version = %u, count = %u)", data_version, count);

	switch (data_version) {
	case SL_MEDIA_CABLE_DB_DATA_VERSION:
		record_size = sizeof(union sl_media_cable_db_record_v1);
		break;
	default:
		sl_media_log_err(media_ldev, LOG_NAME,
				 "cable db load unsupported (data_version = %u)",
				 data_version);
		rtn = -EINVAL;
		goto out_release;
	}

	expected_size = hdr_size + count * record_size;

	if (fw->size < expected_size) {
		sl_media_log_err(media_ldev, LOG_NAME,
				 "cable db load size mismatch (actual = %zu, expected = %u)",
				 fw->size, expected_size);
		rtn = -EINVAL;
		goto out_release;
	}

	sl_media_log_dbg(media_ldev, LOG_NAME,
			 "cable db load size (actual = %lu, exptected = %u)", fw->size, expected_size);

	blob = kmalloc(count * record_size, GFP_KERNEL);
	if (!blob) {
		rtn = -ENOMEM;
		goto out_release;
	}
	memcpy(blob, fw->data + hdr_size, count * record_size);

	media_ldev->cable_db.data         = blob;
	media_ldev->cable_db.data_version = data_version;
	media_ldev->cable_db.record_size  = record_size;
	media_ldev->cable_db.count        = count;

	sl_media_log_dbg(media_ldev, LOG_NAME,
			 "cable db load done (count = %u)", count);
	rtn = 0;

out_release:
	release_firmware(fw);
	return rtn;
}

void sl_media_data_cable_db_unload(struct sl_media_ldev *media_ldev)
{
	if (!media_ldev)
		return;

	sl_media_log_dbg(media_ldev, LOG_NAME, "cable_db unload");

	kfree(media_ldev->cable_db.data);
	media_ldev->cable_db.data         = NULL;
	media_ldev->cable_db.data_version = 0;
	media_ldev->cable_db.record_size  = 0;
	media_ldev->cable_db.count        = 0;
}
