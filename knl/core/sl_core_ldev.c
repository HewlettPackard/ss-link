// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/workqueue.h>
#include <linux/sl_ldev.h>

#include "sl_core_ldev.h"
#include "data/sl_core_data_ldev.h"

int sl_core_ldev_new(u8 ldev_num, struct sl_accessors *accessors,
	struct sl_ops *ops, struct workqueue_struct *workqueue)
{
	return sl_core_data_ldev_new(ldev_num, accessors, ops, workqueue);
}

void sl_core_ldev_del(u8 ldev_num)
{
	sl_core_data_ldev_del(ldev_num);
}

struct sl_core_ldev *sl_core_ldev_get(u8 ldev_num)
{
	return sl_core_data_ldev_get(ldev_num);
}
