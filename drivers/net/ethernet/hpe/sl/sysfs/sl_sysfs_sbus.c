// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static struct attribute *sbus_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(sbus);

static struct kobj_type sbus_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = sbus_groups,
};

int sl_sysfs_sbus_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctrl_lgrp->sbus_kobj, &sbus_info, ctrl_lgrp->parent_kobj, "sbus");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus create kobject_init_and_add failed [%d]", rtn);
		goto out_sbus;
	}

	rtn = sl_sysfs_sbus_rd_create(ctrl_lgrp, &ctrl_lgrp->sbus_kobj);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd create failed [%d]", rtn);
		goto out_sbus;
	}

	rtn = sl_sysfs_sbus_wr_create(ctrl_lgrp, &ctrl_lgrp->sbus_kobj);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus wr create failed [%d]", rtn);
		goto out_sbus;
	}

	rtn = sl_sysfs_sbus_rst_create(ctrl_lgrp, &ctrl_lgrp->sbus_kobj);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rst create failed [%d]", rtn);
		goto out_sbus;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus create (sbus_kobj = 0x%p)", &ctrl_lgrp->sbus_kobj);
	return 0;

out_sbus:
	kobject_put(&ctrl_lgrp->sbus_kobj);

	return -ENOMEM;
}

void sl_sysfs_sbus_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus delete (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj)
		return;

	sl_sysfs_sbus_rst_delete(ctrl_lgrp);
	sl_sysfs_sbus_wr_delete(ctrl_lgrp);
	sl_sysfs_sbus_rd_delete(ctrl_lgrp);

	kobject_put(&ctrl_lgrp->sbus_kobj);
}
