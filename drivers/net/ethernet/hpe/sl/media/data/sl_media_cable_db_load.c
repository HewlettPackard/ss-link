// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/firmware.h>
#include <linux/overflow.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/version.h>

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
#include <asm/unaligned.h>
#else
#include <linux/unaligned.h>
#endif

#include <linux/hpe/sl/sl_media.h>

#include "sl_asic.h"
#include "sl_module.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "data/sl_media_cable_db_firmware.h"
#include "data/sl_media_cable_db_load.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_CABLE_LOG_NAME

int sl_media_data_cable_db_entry_get(struct sl_media_ldev *media_ldev, u32 idx,
				     struct sl_media_cable_attr *attr)
{
	u32                                      data_version;
	const union sl_media_cable_db_record_v1 *v1;

	data_version = media_ldev->cable_db.data_version;

	sl_media_log_dbg(media_ldev, LOG_NAME, "db entry get (idx = %u, data_version = %u)",
			 idx, data_version);

	switch (data_version) {
	case SL_MEDIA_CABLE_DB_DATA_VERSION:
		v1 = media_ldev->cable_db.data + idx * media_ldev->cable_db.record_size;

		attr->hpe_pn    = le32_to_cpu(v1->hpe_pn);
		attr->vendor    = le32_to_cpu(v1->vendor);
		attr->type      = le32_to_cpu(v1->type);
		attr->shape     = le32_to_cpu(v1->shape);
		attr->length_cm = le32_to_cpu(v1->length_cm);
		attr->max_speed = le32_to_cpu(v1->max_speed);

		attr->serdes_settings.pre1   = (s16)le16_to_cpu(v1->serdes_pre1);
		attr->serdes_settings.pre2   = (s16)le16_to_cpu(v1->serdes_pre2);
		attr->serdes_settings.pre3   = (s16)le16_to_cpu(v1->serdes_pre3);
		attr->serdes_settings.cursor = (s16)le16_to_cpu(v1->serdes_cursor);
		attr->serdes_settings.post1  = (s16)le16_to_cpu(v1->serdes_post1);
		attr->serdes_settings.post2  = (s16)le16_to_cpu(v1->serdes_post2);

		attr->fw_ver.major       = v1->fw_ver_major;
		attr->fw_ver.minor       = v1->fw_ver_minor;
		attr->fw_ver.split_major = v1->fw_ver_split_major;
		attr->fw_ver.split_minor = v1->fw_ver_split_minor;

		attr->is_supported_ss200_cable = v1->is_supported_ss200_cable;

		strscpy(attr->vendor_pn_str, v1->vendor_pn_str, sizeof(attr->vendor_pn_str));

		break;
	default:
		sl_media_log_err(media_ldev, LOG_NAME, "db entry get invalid (data_version = %u)",
				 data_version);
		return -EBADRQC;
	}

	return 0;
}

int sl_media_data_cable_db_load(struct sl_media_ldev *media_ldev)
{
	const struct firmware           *fw;
	u32                              magic;
	u32                              hdr_version;
	u32                              data_version;
	u32                              count;
	u32                              date_s;
	u32                              hdr_size;
	u32                              record_size;
	u32                              blob_size;
	u32                              expected_size;
	void                            *blob;
	int                              rtn;
	struct sl_media_cable_db_hdr_v1  hdr_v1;
	struct tm                        db_date;

	sl_media_log_dbg(media_ldev, LOG_NAME, "db load");

	rtn = request_firmware(&fw, SL_MEDIA_CABLE_DB_FW_FILE, sl_device_get());
	if (rtn) {
		sl_media_log_err(media_ldev, LOG_NAME,
				 "db load request_firmware failed [%d]", rtn);
		return -EIO;
	}

	if (fw->size < SL_MEDIA_DB_HDR_SIZE) {
		sl_media_log_err(media_ldev, LOG_NAME,
				 "db load size too small (actual = %zu, minimum = %zu)",
				 fw->size, (size_t)SL_MEDIA_DB_HDR_SIZE);
		rtn = -EINVAL;
		goto out_release;
	}

	magic = le32_to_cpu(get_unaligned_le32(fw->data + SL_MEDIA_CABLE_DB_MAGIC_OFFSET));

	if (magic != SL_MEDIA_CABLE_DB_MAGIC) {
		sl_media_log_err(media_ldev, LOG_NAME,
				 "db load bad magic (actual = 0x%08X, expected = 0x%08X)",
				 magic, SL_MEDIA_CABLE_DB_MAGIC);
		rtn = -EINVAL;
		goto out_release;
	}

	hdr_version = le32_to_cpu(get_unaligned_le32(fw->data + SL_MEDIA_CABLE_DB_HDR_VER_OFFSET));

	sl_media_log_dbg(media_ldev, LOG_NAME, "db load (hdr_version = %u)", hdr_version);

	switch (hdr_version) {
	case SL_MEDIA_CABLE_DB_HDR_VERSION:
		memcpy(&hdr_v1, fw->data, sizeof(hdr_v1));
		data_version = le32_to_cpu(hdr_v1.data_version);
		count        = le32_to_cpu(hdr_v1.count);
		date_s       = le32_to_cpu(hdr_v1.date_s);
		hdr_size     = sizeof(hdr_v1);
		break;
	default:
		sl_media_log_err(media_ldev, LOG_NAME,
				 "db load unsupported (hdr_version = %u)", hdr_version);
		rtn = -EINVAL;
		goto out_release;
	}

	time64_to_tm(date_s, 0, &db_date);

	/* tm_year is years since 1900 */
	sl_media_log_dbg(media_ldev, LOG_NAME,
			 "db load (data_version = %u, count = %u, date = %04ld-%02d-%02d %02d:%02d:%02d)",
			 data_version, count, db_date.tm_year + 1900,
			 db_date.tm_mon + 1, db_date.tm_mday,
			 db_date.tm_hour, db_date.tm_min, db_date.tm_sec);

	switch (data_version) {
	case SL_MEDIA_CABLE_DB_DATA_VERSION:
		record_size = sizeof(union sl_media_cable_db_record_v1);
		break;
	default:
		sl_media_log_err(media_ldev, LOG_NAME,
				 "db load unsupported (data_version = %u)",
				 data_version);
		rtn = -EINVAL;
		goto out_release;
	}

	if (check_mul_overflow(count, record_size, &blob_size) ||
	    check_add_overflow(hdr_size, blob_size, &expected_size)) {
		rtn = -EOVERFLOW;
		goto out_release;
	}

	if (fw->size < expected_size) {
		sl_media_log_err(media_ldev, LOG_NAME,
				 "db load size mismatch (actual = %zu, expected = %u)",
				 fw->size, expected_size);
		rtn = -EINVAL;
		goto out_release;
	}

	sl_media_log_dbg(media_ldev, LOG_NAME,
			 "db load size (actual = %zu, expected = %u)", fw->size, expected_size);

	blob = kmemdup(fw->data + hdr_size, blob_size, GFP_KERNEL);
	if (!blob) {
		rtn = -ENOMEM;
		goto out_release;
	}

	media_ldev->cable_db.data         = blob;
	media_ldev->cable_db.data_version = data_version;
	media_ldev->cable_db.record_size  = record_size;
	media_ldev->cable_db.count        = count;

	sl_media_log_dbg(media_ldev, LOG_NAME,
			 "db load done (count = %u)", count);
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
