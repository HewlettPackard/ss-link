// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static struct attribute *pmi_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(pmi);

static struct kobj_type pmi_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = pmi_groups,
};

int sl_sysfs_pmi_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctrl_lgrp->pmi_kobj, &pmi_info, ctrl_lgrp->parent_kobj, "pmi");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi create kobject_init_and_add failed [%d]", rtn);
		goto out_pmi;
	}

	rtn = sl_sysfs_pmi_rd_create(ctrl_lgrp, &ctrl_lgrp->pmi_kobj);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi rd create failed [%d]", rtn);
		goto out_pmi;
	}

	rtn = sl_sysfs_pmi_wr_create(ctrl_lgrp, &ctrl_lgrp->pmi_kobj);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr create failed [%d]", rtn);
		goto out_pmi;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi create (pmi_kobj = 0x%p)", &ctrl_lgrp->pmi_kobj);
	return 0;

out_pmi:
	kobject_put(&ctrl_lgrp->pmi_kobj);

	return -ENOMEM;
}

void sl_sysfs_pmi_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi delete (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj)
		return;

	sl_sysfs_pmi_wr_delete(ctrl_lgrp);
	sl_sysfs_pmi_rd_delete(ctrl_lgrp);

	kobject_put(&ctrl_lgrp->pmi_kobj);
}
