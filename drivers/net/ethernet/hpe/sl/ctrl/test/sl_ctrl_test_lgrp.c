// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "sl_ctrl_lgrp.h"
#include "test/sl_ctrl_test_lgrp.h"

struct kobject *sl_ctrl_test_lgrp_kobj_get(u8 ldev_num, u8 lgrp_num)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);

	if (!ctrl_lgrp)
		return NULL;

	return ctrl_lgrp->parent_kobj;
}

struct sl_ctrl_lgrp *sl_ctrl_test_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	return sl_ctrl_lgrp_get(ldev_num, lgrp_num);
}
