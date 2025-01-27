// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "sl_ctl_lgrp.h"
#include "test/sl_ctl_test_lgrp.h"

struct kobject *sl_ctl_test_lgrp_kobj_get(u8 ldev_num, u8 lgrp_num)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);

	if (!ctl_lgrp)
		return NULL;

	return ctl_lgrp->parent_kobj;
}

struct sl_ctl_lgrp *sl_ctl_test_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	return sl_ctl_lgrp_get(ldev_num, lgrp_num);
}
