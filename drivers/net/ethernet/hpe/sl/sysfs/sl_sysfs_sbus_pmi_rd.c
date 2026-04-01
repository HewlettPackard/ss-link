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

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_dev_addr_get(core_lgrp, &dev_addr);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd dev_addr show (dev_addr = 0x%02X)", dev_addr);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", dev_addr);
}

static ssize_t addr_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u16                  addr;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_addr_get(core_lgrp, &addr);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd addr show (addr = 0x%04X)", addr);

	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", addr);
}

static ssize_t data_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u16                  data;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_data_get(core_lgrp, &data);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd data show (data = 0x%04X)", data);

	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", data);
}

static ssize_t mask_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u16                  mask;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_mask_get(core_lgrp, &mask);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd mask show (mask = 0x%04X)", mask);

	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", mask);
}

static ssize_t dev_id_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   dev_id;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_dev_id_get(core_lgrp, &dev_id);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd dev id show (dev_id = 0x%02X)", dev_id);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", dev_id);
}

static ssize_t lane_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   lane;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_lane_get(core_lgrp, &lane);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd lane show (lane = 0x%02X)", lane);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", lane);
}

static ssize_t pll_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   pll;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_pll_get(core_lgrp, &pll);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd pll show (pll = 0x%02X)", pll);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", pll);
}

static ssize_t result_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	int                  result;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_pmi_rd_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_pmi_rd_result_get(core_lgrp, &result);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd result show (result = %d)", result);

	return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static struct kobj_attribute dev_addr = __ATTR_RO(dev_addr);
static struct kobj_attribute addr     = __ATTR_RO(addr);
static struct kobj_attribute data     = __ATTR_RO(data);
static struct kobj_attribute mask     = __ATTR_RO(mask);
static struct kobj_attribute dev_id   = __ATTR_RO(dev_id);
static struct kobj_attribute lane     = __ATTR_RO(lane);
static struct kobj_attribute pll      = __ATTR_RO(pll);
static struct kobj_attribute result   = __ATTR_RO(result);

static struct attribute *sbus_pmi_rd_attrs[] = {
	&dev_addr.attr,
	&addr.attr,
	&data.attr,
	&mask.attr,
	&dev_id.attr,
	&lane.attr,
	&pll.attr,
	&result.attr,
	NULL,
};
ATTRIBUTE_GROUPS(sbus_pmi_rd);

static struct kobj_type sbus_pmi_rd_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = sbus_pmi_rd_groups,
};

int sl_sysfs_sbus_pmi_rd_create(struct sl_ctrl_lgrp *ctrl_lgrp, struct kobject *parent_kobj)
{
	int rtn;

	rtn = kobject_init_and_add(&ctrl_lgrp->sbus_pmi_rd_kobj, &sbus_pmi_rd_info, parent_kobj, "rd");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd create kobject_init_and_add failed [%d]", rtn);
		goto out_sbus_pmi_rd;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd create (sbus_pmi_rd_kobj = 0x%p)",
		   &ctrl_lgrp->sbus_pmi_rd_kobj);
	return 0;

out_sbus_pmi_rd:
	kobject_put(&ctrl_lgrp->sbus_pmi_rd_kobj);

	return -ENOMEM;
}

void sl_sysfs_sbus_pmi_rd_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus_pmi rd delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&ctrl_lgrp->sbus_pmi_rd_kobj);
}
