/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_LDEV_H_
#define _SL_MEDIA_DATA_LDEV_H_

#include <linux/types.h>

#define SL_MEDIA_HIGH_TEMP_MONITOR_TIME_MS 15000

struct sl_media_ldev;
struct sl_uc_ops;
struct sl_uc_accessor;

int                   sl_media_data_ldev_new(u8 ldev_num, struct workqueue_struct *workqueue);
void                  sl_media_data_ldev_del(u8 ldev_num);
struct sl_media_ldev *sl_media_data_ldev_get(u8 ldev_num);

int sl_media_data_ldev_uc_ops_set(u8 ldev_num, struct sl_uc_ops *uc_ops,
				  struct sl_uc_accessor *uc_accessor);

#endif /* _SL_MEDIA_DATA_LDEV_H_ */
