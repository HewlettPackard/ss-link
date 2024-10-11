// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_media_ldev.h"
#include "data/sl_media_data_ldev.h"

int sl_media_ldev_new(u8 ldev_num)
{
	return sl_media_data_ldev_new(ldev_num);
}

void sl_media_ldev_del(u8 ldev_num)
{
	sl_media_data_ldev_del(ldev_num);
}

struct sl_media_ldev *sl_media_ldev_get(u8 ldev_num)
{
	return sl_media_data_ldev_get(ldev_num);
}

int sl_media_ldev_uc_ops_set(u8 ldev_num, struct sl_uc_ops *uc_ops,
				struct sl_uc_accessor *uc_accessor)
{
	return sl_media_data_ldev_uc_ops_set(ldev_num, uc_ops, uc_accessor);
}
