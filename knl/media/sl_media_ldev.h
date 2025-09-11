/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_LDEV_H_
#define _SL_MEDIA_LDEV_H_

#include <linux/types.h>

#define SL_MEDIA_LDEV_MAGIC 0x736c4D44

struct sl_uc_ops;
struct sl_uc_accessor;

struct sl_media_ldev {
	u32                    magic;
	u8                     num;
	struct sl_uc_ops      *uc_ops;
	struct sl_uc_accessor *uc_accessor;
};

int                   sl_media_ldev_new(u8 ldev_num);
void                  sl_media_ldev_del(u8 ldev_num);
struct sl_media_ldev *sl_media_ldev_get(u8 ldev_num);

int sl_media_ldev_uc_ops_set(u8 ldev_num, struct sl_uc_ops *uc_ops,
			     struct sl_uc_accessor *uc_accessor);

#endif /* _SL_MEDIA_LDEV_H_ */
