// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_core_link.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"
#include "sl_sysfs_link_fec_current.h"
#include "sl_sysfs_link_fec_mon_check.h"
#include "sl_sysfs_link_fec_up_check.h"
#include "sl_sysfs_link_fec_up.h"
#include "sl_sysfs_link_fec_down.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static struct attribute *link_fec_attrs[] = {
	NULL
};
ATTRIBUTE_GROUPS(link_fec);

static struct kobj_type link_fec = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_groups,
};

int sl_sysfs_link_fec_create(struct sl_ctrl_link *ctrl_link)
{
	int                  rtn;
	struct sl_core_link *core_link;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link fec create (num = %u)", ctrl_link->num);

	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	rtn = kobject_init_and_add(&ctrl_link->fec.kobj, &link_fec, &ctrl_link->kobj, "fec");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link fec create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->fec.kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_fec_current_create(core_link, &ctrl_link->fec.kobj);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link fec current create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->fec.kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_fec_mon_check_create(ctrl_link);
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME, "fec mon check create failed [%d]", rtn);
		sl_sysfs_link_fec_current_delete(core_link);
		kobject_put(&ctrl_link->fec.kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_fec_up_check_create(core_link, &ctrl_link->fec.kobj);
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME, "fec up check create failed [%d]", rtn);
		sl_sysfs_link_fec_current_delete(core_link);
		sl_sysfs_link_fec_mon_check_delete(ctrl_link);
		kobject_put(&ctrl_link->fec.kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_fec_up_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link fec up create kobject_init_and_add failed [%d]", rtn);
		sl_sysfs_link_fec_current_delete(core_link);
		sl_sysfs_link_fec_mon_check_delete(ctrl_link);
		sl_sysfs_link_fec_up_check_delete(core_link);
		kobject_put(&ctrl_link->fec.kobj);
		return rtn;
	}

	rtn = sl_sysfs_link_fec_down_create(ctrl_link);
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link fec down create kobject_init_and_add failed [%d]", rtn);
		sl_sysfs_link_fec_current_delete(core_link);
		sl_sysfs_link_fec_mon_check_delete(ctrl_link);
		sl_sysfs_link_fec_up_check_delete(core_link);
		sl_sysfs_link_fec_up_delete(ctrl_link);
		kobject_put(&ctrl_link->fec.kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_fec_delete(struct sl_ctrl_link *ctrl_link)
{
	struct sl_core_link *core_link;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link fec delete (num = %u)", ctrl_link->num);

	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	sl_sysfs_link_fec_current_delete(core_link);
	sl_sysfs_link_fec_mon_check_delete(ctrl_link);
	sl_sysfs_link_fec_up_check_delete(core_link);
	sl_sysfs_link_fec_up_delete(ctrl_link);
	sl_sysfs_link_fec_down_delete(ctrl_link);
	kobject_put(&ctrl_link->fec.kobj);
}
