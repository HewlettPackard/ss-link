// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t dev_addr_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   dev_addr;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rd_dev_addr_get(core_lgrp, &dev_addr);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd dev addr show (dev_addr = 0x%02X)", dev_addr);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", dev_addr);
}

static ssize_t data_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u32                  data;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rd_data_get(core_lgrp, &data);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd data show (data = 0x%08X)", data);

	return scnprintf(buf, PAGE_SIZE, "0x%08X\n", data);
}

static ssize_t mask_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u32                  mask;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rd_mask_get(core_lgrp, &mask);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd mask show (mask = 0x%08X)", mask);

	return scnprintf(buf, PAGE_SIZE, "0x%08X\n", mask);
}

static ssize_t reg_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   reg;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rd_reg_get(core_lgrp, &reg);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd reg show (reg = 0x%02X)", reg);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", reg);
}

static ssize_t lsb_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   lsb;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rd_lsb_get(core_lgrp, &lsb);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd lsb show (lsb = %u)", lsb);

	return scnprintf(buf, PAGE_SIZE, "%u\n", lsb);
}

static ssize_t result_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	int                  result;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rd_result_get(core_lgrp, &result);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd result show (result = %d)", result);

	return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static struct kobj_attribute dev_addr = __ATTR_RO(dev_addr);
static struct kobj_attribute data     = __ATTR_RO(data);
static struct kobj_attribute mask     = __ATTR_RO(mask);
static struct kobj_attribute reg      = __ATTR_RO(reg);
static struct kobj_attribute lsb      = __ATTR_RO(lsb);
static struct kobj_attribute result   = __ATTR_RO(result);

static struct attribute *sbus_rd_attrs[] = {
	&dev_addr.attr,
	&data.attr,
	&mask.attr,
	&reg.attr,
	&lsb.attr,
	&result.attr,
	NULL,
};
ATTRIBUTE_GROUPS(sbus_rd);

static struct kobj_type sbus_rd_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = sbus_rd_groups,
};

int sl_sysfs_sbus_rd_create(struct sl_ctrl_lgrp *ctrl_lgrp, struct kobject *parent_kobj)
{
	int rtn;

	rtn = kobject_init_and_add(&ctrl_lgrp->sbus_rd_kobj, &sbus_rd_info, parent_kobj, "rd");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd create kobject_init_and_add failed [%d]", rtn);
		goto out_sbus_rd;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd create (sbus_rd_kobj = 0x%p)", &ctrl_lgrp->sbus_rd_kobj);
	return 0;

out_sbus_rd:
	kobject_put(&ctrl_lgrp->sbus_rd_kobj);

	return -ENOMEM;
}

void sl_sysfs_sbus_rd_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rd delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&ctrl_lgrp->sbus_rd_kobj);
}

