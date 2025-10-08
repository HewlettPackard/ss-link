// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_str.h"
#include "sl_sysfs_serdes_settings.h"
#include "sl_sysfs_serdes_eye.h"
#include "sl_sysfs_serdes_state.h"
#include "sl_sysfs_serdes_swizzle.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

u8 lane_num_to_link_num(struct sl_ctrl_lgrp *ctrl_lgrp, u8 lane_num)
{
	switch (ctrl_lgrp->config.furcation) {
	case SL_MEDIA_FURCATION_X1:
	default:
		return 0;
	case SL_MEDIA_FURCATION_X2:
		if (lane_num & BIT(1))
			return 1;
		else
			return 0;
	case SL_MEDIA_FURCATION_X4:
		return lane_num;
	}
}

static ssize_t hw_rev_1_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, serdes_kobj);
	if (!ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-group\n");

	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"hw ver show (hw_rev_1 = 0x%X)",
		core_lgrp->core_ldev->serdes.hw_info[LGRP_TO_SERDES(core_lgrp->num)].rev_id_1);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n",
		core_lgrp->core_ldev->serdes.hw_info[LGRP_TO_SERDES(core_lgrp->num)].rev_id_1);
}

static ssize_t hw_rev_2_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, serdes_kobj);
	if (!ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-group\n");

	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"hw ver show (hw_rev_2 = 0x%X)",
		core_lgrp->core_ldev->serdes.hw_info[LGRP_TO_SERDES(core_lgrp->num)].rev_id_2);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n",
		core_lgrp->core_ldev->serdes.hw_info[LGRP_TO_SERDES(core_lgrp->num)].rev_id_2);
}

static ssize_t hw_version_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, serdes_kobj);
	if (!ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-group\n");

	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"hw ver show (hw_version = 0x%X)",
		core_lgrp->core_ldev->serdes.hw_info[LGRP_TO_SERDES(core_lgrp->num)].version);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n",
		core_lgrp->core_ldev->serdes.hw_info[LGRP_TO_SERDES(core_lgrp->num)].version);
}

static ssize_t fw_signature_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, serdes_kobj);
	if (!ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-group\n");

	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"fw signature show (fw_signature = 0x%X)",
		core_lgrp->core_ldev->serdes.fw_info[LGRP_TO_SERDES(core_lgrp->num)].signature);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n",
		core_lgrp->core_ldev->serdes.fw_info[LGRP_TO_SERDES(core_lgrp->num)].signature);
}

static ssize_t fw_version_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, serdes_kobj);
	if (!ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-group\n");

	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"fw version show (fw_version = 0x%X)",
		core_lgrp->core_ldev->serdes.fw_info[LGRP_TO_SERDES(core_lgrp->num)].version);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n",
		core_lgrp->core_ldev->serdes.fw_info[LGRP_TO_SERDES(core_lgrp->num)].version);
}

static struct kobj_attribute hw_rev_id_1   = __ATTR_RO(hw_rev_1);
static struct kobj_attribute hw_rev_id_2   = __ATTR_RO(hw_rev_2);
static struct kobj_attribute hw_version    = __ATTR_RO(hw_version);
static struct kobj_attribute fw_signature  = __ATTR_RO(fw_signature);
static struct kobj_attribute fw_version    = __ATTR_RO(fw_version);

static struct attribute *serdes_attrs[] = {
	&hw_rev_id_1.attr,
	&hw_rev_id_2.attr,
	&hw_version.attr,
	&fw_signature.attr,
	&fw_version.attr,
	NULL
};
ATTRIBUTE_GROUPS(serdes);

static struct kobj_type serdes_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = serdes_groups,
};

static struct attribute *serdes_lane_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(serdes_lane);

static struct kobj_type serdes_lane_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = serdes_lane_groups,
};

int sl_sysfs_serdes_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int rtn;
	int asic_lane_num;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "serdes create (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "serdes create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&(ctrl_lgrp->serdes_kobj),
		&serdes_info, ctrl_lgrp->parent_kobj, "serdes");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			"serdes create kobject_init_and_add failed [%d]", rtn);
		goto out_serdes;
	}

	rtn = kobject_init_and_add(&(ctrl_lgrp->serdes_lane_kobj),
		&serdes_lane_info, &(ctrl_lgrp->serdes_kobj), "lane");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			"serdes create kobject_init_and_add failed [%d]", rtn);
		goto out_lane;
	}

	for (asic_lane_num = 0; asic_lane_num < SL_ASIC_MAX_LANES; ++asic_lane_num) {

		rtn = kobject_init_and_add(&(ctrl_lgrp->serdes_lane_kobjs[asic_lane_num]),
			&serdes_lane_info, &(ctrl_lgrp->serdes_lane_kobj), "%u", asic_lane_num);
		if (rtn) {
			sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				"serdes create lane kobject_init_and_add failed [%d]", rtn);
			goto out_all;
		}

		rtn = sl_sysfs_serdes_lane_state_create(ctrl_lgrp, asic_lane_num);
		if (rtn) {
			sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				"serdes_lane_state_create failed [%d]", rtn);
			goto out_lanes;
		}

		rtn = sl_sysfs_serdes_lane_swizzle_create(ctrl_lgrp, asic_lane_num);
		if (rtn) {
			sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				"serdes_lane_swizzle_create failed [%d]", rtn);
			goto out_swizzle;
		}

		rtn = sl_sysfs_serdes_lane_settings_create(ctrl_lgrp, asic_lane_num);
		if (rtn) {
			sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				"serdes_lane_settings_create failed [%d]", rtn);
			goto out_state;
		}

		rtn = sl_sysfs_serdes_lane_eye_create(ctrl_lgrp, asic_lane_num);
		if (rtn) {
			sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				"serdes_lane_eye_create failed [%d]", rtn);
			goto out_settings;
		}
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes create (serdes_kobj = 0x%p)", &(ctrl_lgrp->serdes_kobj));
	return 0;

out_settings:
	sl_sysfs_serdes_lane_settings_delete(ctrl_lgrp, asic_lane_num);
out_state:
	sl_sysfs_serdes_lane_state_delete(ctrl_lgrp, asic_lane_num);
out_swizzle:
	sl_sysfs_serdes_lane_swizzle_delete(ctrl_lgrp, asic_lane_num);
out_lanes:
	kobject_put(&(ctrl_lgrp->serdes_lane_kobjs[asic_lane_num]));
out_all:
	for (asic_lane_num = asic_lane_num - 1; asic_lane_num >= 0; --asic_lane_num) {
		sl_sysfs_serdes_lane_eye_delete(ctrl_lgrp, asic_lane_num);
		sl_sysfs_serdes_lane_settings_delete(ctrl_lgrp, asic_lane_num);
		sl_sysfs_serdes_lane_swizzle_delete(ctrl_lgrp, asic_lane_num);
		sl_sysfs_serdes_lane_state_delete(ctrl_lgrp, asic_lane_num);
		kobject_put(&(ctrl_lgrp->serdes_lane_kobjs[asic_lane_num]));
	}
out_lane:
	kobject_put(&(ctrl_lgrp->serdes_lane_kobj));
out_serdes:
	kobject_put(&(ctrl_lgrp->serdes_kobj));

	return -ENOMEM;
}

void sl_sysfs_serdes_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	u8 asic_lane_num;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "serdes delete (lgrp = 0x%p)", ctrl_lgrp);

	if (!ctrl_lgrp->parent_kobj)
		return;

	for (asic_lane_num = 0; asic_lane_num < SL_ASIC_MAX_LANES; ++asic_lane_num) {
		sl_sysfs_serdes_lane_eye_delete(ctrl_lgrp, asic_lane_num);
		sl_sysfs_serdes_lane_settings_delete(ctrl_lgrp, asic_lane_num);
		sl_sysfs_serdes_lane_swizzle_delete(ctrl_lgrp, asic_lane_num);
		sl_sysfs_serdes_lane_state_delete(ctrl_lgrp, asic_lane_num);
		kobject_put(&(ctrl_lgrp->serdes_lane_kobjs[asic_lane_num]));
	}
	kobject_put(&(ctrl_lgrp->serdes_lane_kobj));
	kobject_put(&ctrl_lgrp->serdes_kobj);
}
