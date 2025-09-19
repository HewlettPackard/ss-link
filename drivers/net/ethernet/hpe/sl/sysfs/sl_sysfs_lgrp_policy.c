// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static struct attribute *lgrp_policy_attrs[] = {
	NULL
};
ATTRIBUTE_GROUPS(lgrp_policy);

static struct kobj_type policy_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = lgrp_policy_groups,
};

int sl_sysfs_lgrp_policy_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp policy create (lgrp = 0x%p)", ctrl_lgrp);

	rtn = kobject_init_and_add(&(ctrl_lgrp->policy_kobj), &policy_info, ctrl_lgrp->parent_kobj, "policies");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp policy create failed [%d]", rtn);
		kobject_put(&(ctrl_lgrp->policy_kobj));
		return -ENOMEM;
	}

	return 0;
}

void sl_sysfs_lgrp_policy_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp policy delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&(ctrl_lgrp->policy_kobj));
}
