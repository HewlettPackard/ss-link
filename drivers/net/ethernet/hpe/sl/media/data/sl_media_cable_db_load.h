/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_CABLE_DB_LOAD_H_
#define _SL_MEDIA_CABLE_DB_LOAD_H_

struct sl_media_ldev;
struct sl_media_jack;
struct sl_media_cable_attr;

/**
 * sl_media_data_cable_db_load() - Load cable DB from firmware binary.
 * @media_ldev: the media link device to populate
 *
 * Returns 0 on success or a negative errno on failure.
 */
int  sl_media_data_cable_db_load(struct sl_media_ldev *media_ldev);

/**
 * sl_media_data_cable_db_unload() - Free the loaded cable DB.
 * @media_ldev: the media link device whose DB should be freed
 */
void sl_media_data_cable_db_unload(struct sl_media_ldev *media_ldev);

/**
 * sl_media_data_cable_db_entry_get() - Fill attr from the DB entry for a jack.
 * @media_jack: jack whose cable_db_idx has been set
 * @attr: output struct to fill
 */
void sl_media_data_cable_db_entry_get(const struct sl_media_jack *media_jack,
				      struct sl_media_cable_attr *attr);

/**
 * sl_media_data_cable_db_entry_get_by_idx() - Fill attr from DB entry at index.
 * @media_ldev: the media link device
 * @idx: record index
 * @attr: output struct to fill
 */
void sl_media_data_cable_db_entry_get_by_idx(const struct sl_media_ldev *media_ldev,
					     u32 idx, struct sl_media_cable_attr *attr);

#endif /* _SL_MEDIA_CABLE_DB_LOAD_H_ */
