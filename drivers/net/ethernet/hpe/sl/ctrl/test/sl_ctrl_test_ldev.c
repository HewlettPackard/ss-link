// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_asic.h"
#include "sl_ctrl_ldev.h"
#include "test/sl_ctrl_test_ldev.h"

struct kobject *sl_ctrl_test_ldev_kobj_get(u8 ldev_num)
{
	struct sl_ctrl_ldev *ctrl_ldev;
	struct kobject      *parent_kobj;

	ctrl_ldev = sl_ctrl_ldev_get_ref(ldev_num);
	if (!ctrl_ldev)
		return NULL;

	parent_kobj = ctrl_ldev->parent_kobj;
	sl_ctrl_ldev_put_ref(ctrl_ldev);

	return parent_kobj;
}

struct sl_ctrl_ldev *sl_ctrl_test_ldev_get(u8 ldev_num)
{
	return sl_ctrl_ldev_get_ref(ldev_num);
}

void sl_ctrl_test_ldev_put_ref(struct sl_ctrl_ldev *ctrl_ldev)
{
	sl_ctrl_ldev_put_ref(ctrl_ldev);
}
