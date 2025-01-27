// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_ctl_ldev.h"
#include "test/sl_ctl_test_ldev.h"

struct kobject *sl_ctl_test_ldev_kobj_get(u8 ldev_num)
{
	struct sl_ctl_ldev *ctl_ldev;

	ctl_ldev = sl_ctl_ldev_get(ldev_num);

	if (!ctl_ldev)
		return NULL;

	return ctl_ldev->parent_kobj;
}

struct sl_ctl_ldev *sl_ctl_test_ldev_get(u8 ldev_num)
{
	return sl_ctl_ldev_get(ldev_num);
}
