// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_str.h"
#include "sl_sysfs_serdes_swizzle.h"
#include "hw/sl_core_hw_serdes_lane.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t tx_source_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	u8                               tx_source;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	tx_source = core_lgrp->serdes.dt.lane_info[lane_kobj->asic_lane_num].tx_source;

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "tx_source show (tx_source = %u)", tx_source);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tx_source);
}

static ssize_t rx_source_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	u8                               rx_source;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	rx_source = core_lgrp->serdes.dt.lane_info[lane_kobj->asic_lane_num].rx_source;

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "rx_source show (rx_source = %u)", rx_source);

	return scnprintf(buf, PAGE_SIZE, "%u\n", rx_source);
}

static struct kobj_attribute tx_source = __ATTR_RO(tx_source);
static struct kobj_attribute rx_source = __ATTR_RO(rx_source);

static struct attribute *serdes_lane_swizzle_attrs[] = {
	&tx_source.attr,
	&rx_source.attr,
	NULL,
};
ATTRIBUTE_GROUPS(serdes_lane_swizzle);

static struct kobj_type serdes_lane_swizzle_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = serdes_lane_swizzle_groups,
};

int sl_sysfs_serdes_lane_swizzle_create(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane swizzle create (lgrp = 0x%p)", ctrl_lgrp);

	rtn = kobject_init_and_add(&(ctrl_lgrp->serdes_lane_swizzle_kobjs[asic_lane_num].kobj),
		&serdes_lane_swizzle_info, &(ctrl_lgrp->serdes_lane_kobjs[asic_lane_num]), "swizzle");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			"serdes lane swizzle create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&(ctrl_lgrp->serdes_lane_swizzle_kobjs[asic_lane_num].kobj));
		return -ENOMEM;
	}
	ctrl_lgrp->serdes_lane_swizzle_kobjs[asic_lane_num].ctrl_lgrp      = ctrl_lgrp;
	ctrl_lgrp->serdes_lane_swizzle_kobjs[asic_lane_num].asic_lane_num = asic_lane_num;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane swizzle create (serdes_kobj = 0x%p)", &(ctrl_lgrp->serdes_kobj));
	return 0;
}

void sl_sysfs_serdes_lane_swizzle_delete(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane swizzle delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&(ctrl_lgrp->serdes_lane_swizzle_kobjs[asic_lane_num].kobj));
	ctrl_lgrp->serdes_lane_swizzle_kobjs[asic_lane_num].ctrl_lgrp = NULL;
}
