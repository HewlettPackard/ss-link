/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_CABLE_DB_OPS_H_
#define _SL_MEDIA_DATA_CABLE_DB_OPS_H_

struct sl_media_attr;
struct sl_media_jack;

int sl_media_data_cable_db_ops_cable_validate(struct sl_media_attr *media_attr, struct sl_media_jack *media_jack);
int sl_media_data_cable_db_ops_serdes_settings_get(struct sl_media_jack *media_jack, u32 media_type, u32 flags);

#endif /* _SL_MEDIA_DATA_CABLE_DB_OPS_H_ */
