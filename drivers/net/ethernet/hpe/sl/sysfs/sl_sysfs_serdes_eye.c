// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_sysfs_serdes_eye.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t value_upper_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                              rtn;
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	u8                               eye_upper;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	rtn = sl_core_lgrp_eye_upper_get(core_lgrp, lane_kobj->asic_lane_num, &eye_upper);
	if (rtn == -EIO)
		return scnprintf(buf, PAGE_SIZE, "no-serdes\n");
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"eye value upper show (asic_lane_num = %u, eye = %u)",
		lane_kobj->asic_lane_num, eye_upper);

	return scnprintf(buf, PAGE_SIZE, "%u\n", eye_upper);
}

static ssize_t value_lower_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                              rtn;
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	u8                               eye_lower;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	rtn = sl_core_lgrp_eye_lower_get(core_lgrp, lane_kobj->asic_lane_num, &eye_lower);
	if (rtn == -EIO)
		return scnprintf(buf, PAGE_SIZE, "no-serdes\n");
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"eye value lower show (asic_lane_num = %u, eye = %u)",
		lane_kobj->asic_lane_num, eye_lower);

	return scnprintf(buf, PAGE_SIZE, "%u\n", eye_lower);
}

static ssize_t limit_high_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"eye limit high show (asic_lane_num = %u, limit = %u)",
		lane_kobj->asic_lane_num, core_lgrp->serdes.eye_limits[lane_kobj->asic_lane_num].high);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
		core_lgrp->serdes.eye_limits[lane_kobj->asic_lane_num].high);
}

static ssize_t limit_low_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"eye limit low show (asic_lane_num = %u, limit = %u)",
		lane_kobj->asic_lane_num, core_lgrp->serdes.eye_limits[lane_kobj->asic_lane_num].low);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
		core_lgrp->serdes.eye_limits[lane_kobj->asic_lane_num].low);
}

static struct kobj_attribute eye_value_upper = __ATTR_RO(value_upper);
static struct kobj_attribute eye_value_lower = __ATTR_RO(value_lower);
static struct kobj_attribute eye_limit_high  = __ATTR_RO(limit_high);
static struct kobj_attribute eye_limit_low   = __ATTR_RO(limit_low);

static struct attribute *serdes_lane_eye_attrs[] = {
	&eye_value_upper.attr,
	&eye_value_lower.attr,
	&eye_limit_high.attr,
	&eye_limit_low.attr,
	NULL,
};
ATTRIBUTE_GROUPS(serdes_lane_eye);

static struct kobj_type serdes_lane_eye_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = serdes_lane_eye_groups,
};

int sl_sysfs_serdes_lane_eye_create(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane eye create (lgrp = 0x%p)", ctrl_lgrp);

	rtn = kobject_init_and_add(&(ctrl_lgrp->serdes_lane_eye_kobjs[asic_lane_num].kobj),
		&serdes_lane_eye_info, &(ctrl_lgrp->serdes_lane_kobjs[asic_lane_num]), "eye");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			"serdes lane eye create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&(ctrl_lgrp->serdes_lane_eye_kobjs[asic_lane_num].kobj));
		return -ENOMEM;
	}
	ctrl_lgrp->serdes_lane_eye_kobjs[asic_lane_num].ctrl_lgrp      = ctrl_lgrp;
	ctrl_lgrp->serdes_lane_eye_kobjs[asic_lane_num].asic_lane_num = asic_lane_num;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane eye create (serdes_kobj = 0x%p)", &(ctrl_lgrp->serdes_kobj));
	return 0;
}

void sl_sysfs_serdes_lane_eye_delete(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane eye delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&(ctrl_lgrp->serdes_lane_eye_kobjs[asic_lane_num].kobj));
	ctrl_lgrp->serdes_lane_eye_kobjs[asic_lane_num].ctrl_lgrp = NULL;
}
