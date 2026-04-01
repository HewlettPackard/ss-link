// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static struct attribute *sbus_pmi_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(sbus_pmi);

static struct kobj_type sbus_pmi_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = sbus_pmi_groups,
};

int sl_sysfs_sbus_pmi_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi create (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctrl_lgrp->sbus_pmi_kobj, &sbus_pmi_info, ctrl_lgrp->parent_kobj, "sbus_pmi");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi create kobject_init_and_add failed [%d]", rtn);
		goto out_sbus_pmi;
	}

	rtn = sl_sysfs_sbus_pmi_rd_create(ctrl_lgrp, &ctrl_lgrp->sbus_pmi_kobj);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd create failed [%d]", rtn);
		goto out_sbus_pmi;
	}

	rtn = sl_sysfs_sbus_pmi_wr_create(ctrl_lgrp, &ctrl_lgrp->sbus_pmi_kobj);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi wr create failed [%d]", rtn);
		goto out_sbus_pmi;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi create (sbus_pmi_kobj = 0x%p)", &ctrl_lgrp->sbus_pmi_kobj);
	return 0;

out_sbus_pmi:
	kobject_put(&ctrl_lgrp->sbus_pmi_kobj);

	return -ENOMEM;
}

void sl_sysfs_sbus_pmi_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi delete (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj)
		return;

	sl_sysfs_sbus_pmi_wr_delete(ctrl_lgrp);
	sl_sysfs_sbus_pmi_rd_delete(ctrl_lgrp);

	kobject_put(&ctrl_lgrp->sbus_pmi_kobj);
}
