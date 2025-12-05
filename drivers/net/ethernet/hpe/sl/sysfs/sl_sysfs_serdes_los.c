// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "asm-generic/int-ll64.h"
#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_str.h"
#include "sl_sysfs_serdes_los.h"
#include "hw/sl_core_hw_serdes_lane.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t tx_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                              rtn;
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	bool                             is_tx_los;
	struct sl_ctrl_lgrp             *ctrl_lgrp;
	u8                               ldev_num;
	u8                               lgrp_num;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	ctrl_lgrp = lane_kobj->ctrl_lgrp;
	ldev_num = lane_kobj->ctrl_lgrp->ctrl_ldev->num;
	lgrp_num = lane_kobj->ctrl_lgrp->num;

	rtn = sl_core_lgrp_tx_lane_is_los(ldev_num, lgrp_num, lane_kobj->asic_lane_num, &is_tx_los);
	if (rtn == -ENOENT) {
		sl_core_log_dbg(ctrl_lgrp, LOG_NAME,
				"los tx show (asic_lane_num = %u) - tx_lane_is_los no cache [%d]",
				lane_kobj->asic_lane_num, rtn);
		return scnprintf(buf, PAGE_SIZE, "no-cache\n");
	}

	if (rtn) {
		sl_core_log_err_trace(ctrl_lgrp, LOG_NAME,
				      "los tx show (asic_lane_num = %u) - tx_lane_is_los failed [%d]",
				      lane_kobj->asic_lane_num, rtn);
		return scnprintf(buf, PAGE_SIZE, "error\n");
	}

	sl_core_log_dbg(ctrl_lgrp, LOG_NAME,
			"los tx show (asic_lane_num = %u, is_tx_los = %s)",
			lane_kobj->asic_lane_num, is_tx_los ? "yes" : "no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_tx_los ? "yes" : "no");
}

static ssize_t rx_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                              rtn;
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	bool                             is_rx_los;
	struct sl_ctrl_lgrp             *ctrl_lgrp;
	u8                               ldev_num;
	u8                               lgrp_num;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	ctrl_lgrp = lane_kobj->ctrl_lgrp;
	ldev_num = lane_kobj->ctrl_lgrp->ctrl_ldev->num;
	lgrp_num = lane_kobj->ctrl_lgrp->num;

	rtn = sl_core_lgrp_rx_lane_is_los(ldev_num, lgrp_num, lane_kobj->asic_lane_num, &is_rx_los);
	if (rtn == -ENOENT) {
		sl_core_log_dbg(ctrl_lgrp, LOG_NAME,
				"los rx show (asic_lane_num = %u) - rx_lane_is_los no cache [%d]",
				lane_kobj->asic_lane_num, rtn);
		return scnprintf(buf, PAGE_SIZE, "no-cache\n");
	}

	if (rtn) {
		sl_core_log_err_trace(ctrl_lgrp, LOG_NAME,
				      "los rx show (asic_lane_num = %u) - rx_lane_is_los failed [%d]",
				      lane_kobj->asic_lane_num, rtn);
		return scnprintf(buf, PAGE_SIZE, "error\n");
	}

	sl_core_log_dbg(ctrl_lgrp, LOG_NAME,
			"los rx show (asic_lane_num = %u, is_rx_los = %s)",
			lane_kobj->asic_lane_num, is_rx_los ? "yes" : "no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_rx_los ? "yes" : "no");
}

static struct kobj_attribute los_tx = __ATTR_RO(tx);
static struct kobj_attribute los_rx = __ATTR_RO(rx);

static struct attribute *serdes_lane_los_attrs[] = {
	&los_tx.attr,
	&los_rx.attr,
	NULL,
};
ATTRIBUTE_GROUPS(serdes_lane_los);

static struct kobj_type serdes_lane_los_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = serdes_lane_los_groups,
};

int sl_sysfs_serdes_lane_los_create(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		   "serdes lane los create (lgrp = 0x%p)", ctrl_lgrp);

	rtn = kobject_init_and_add(&ctrl_lgrp->serdes_lane_los_kobjs[asic_lane_num].kobj,
				   &serdes_lane_los_info, &ctrl_lgrp->serdes_lane_kobjs[asic_lane_num], "los");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			   "serdes lane los create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_lgrp->serdes_lane_los_kobjs[asic_lane_num].kobj);
		return -ENOMEM;
	}
	ctrl_lgrp->serdes_lane_los_kobjs[asic_lane_num].ctrl_lgrp     = ctrl_lgrp;
	ctrl_lgrp->serdes_lane_los_kobjs[asic_lane_num].asic_lane_num = asic_lane_num;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		   "serdes lane los create (serdes_kobj = 0x%p)", &ctrl_lgrp->serdes_kobj);
	return 0;
}

void sl_sysfs_serdes_lane_los_delete(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		   "serdes lane los delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&ctrl_lgrp->serdes_lane_los_kobjs[asic_lane_num].kobj);
	ctrl_lgrp->serdes_lane_los_kobjs[asic_lane_num].ctrl_lgrp = NULL;
}
