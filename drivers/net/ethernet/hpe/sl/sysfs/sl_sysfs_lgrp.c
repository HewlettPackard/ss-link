// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

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

int sl_sysfs_lgrp_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp create (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp create missing parent");
		return -EBADRQC;
	}

	rtn = sl_sysfs_lgrp_config_create(ctrl_lgrp);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "config create failed [%d]", rtn);
		return -ENOMEM;
	}

	rtn = sl_sysfs_lgrp_policy_create(ctrl_lgrp);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "policy create failed [%d]", rtn);
		return -ENOMEM;
	}

	rtn = sl_sysfs_serdes_create(ctrl_lgrp);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "serdes create failed [%d]", rtn);
		return -ENOMEM;
	}

	rtn = sl_sysfs_media_create(ctrl_lgrp);
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "media create failed [%d]", rtn);
		return -ENOMEM;
	}

	return 0;
}

void sl_sysfs_lgrp_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp delete (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj)
		return;

	sl_sysfs_media_delete(ctrl_lgrp);
	sl_sysfs_serdes_delete(ctrl_lgrp);
	sl_sysfs_lgrp_policy_delete(ctrl_lgrp);
	sl_sysfs_lgrp_config_delete(ctrl_lgrp);
}
