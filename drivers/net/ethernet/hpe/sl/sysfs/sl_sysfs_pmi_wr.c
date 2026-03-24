// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t addr_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u16                  addr;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, pmi_wr_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_pmi_wr_addr_get(core_lgrp, &addr);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr addr show (addr = 0x%04X)", addr);

	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", addr);
}

static ssize_t data_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u16                  data;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, pmi_wr_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_pmi_wr_data_get(core_lgrp, &data);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr data show (data = 0x%04X)", data);

	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", data);
}

static ssize_t mask_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u16                  mask;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, pmi_wr_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_pmi_wr_mask_get(core_lgrp, &mask);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr mask show (mask = 0x%04X)", mask);

	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", mask);
}

static ssize_t dev_id_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   dev_id;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, pmi_wr_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_pmi_wr_dev_id_get(core_lgrp, &dev_id);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr dev id show (dev_id = 0x%02X)", dev_id);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", dev_id);
}

static ssize_t lane_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   lane;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, pmi_wr_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_pmi_wr_lane_get(core_lgrp, &lane);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr lane show (lane = 0x%02X)", lane);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", lane);
}

static ssize_t pll_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	u8                   pll;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, pmi_wr_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_pmi_wr_pll_get(core_lgrp, &pll);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr pll show (pll = 0x%02X)", pll);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", pll);
}

static ssize_t result_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	int                  result;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, pmi_wr_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_pmi_wr_result_get(core_lgrp, &result);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr result show (result = %d)", result);

	return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static struct kobj_attribute addr   = __ATTR_RO(addr);
static struct kobj_attribute data   = __ATTR_RO(data);
static struct kobj_attribute mask   = __ATTR_RO(mask);
static struct kobj_attribute dev_id = __ATTR_RO(dev_id);
static struct kobj_attribute lane   = __ATTR_RO(lane);
static struct kobj_attribute pll    = __ATTR_RO(pll);
static struct kobj_attribute result = __ATTR_RO(result);

static struct attribute *pmi_wr_attrs[] = {
	&addr.attr,
	&data.attr,
	&mask.attr,
	&dev_id.attr,
	&lane.attr,
	&pll.attr,
	&result.attr,
	NULL,
};
ATTRIBUTE_GROUPS(pmi_wr);

static struct kobj_type pmi_wr_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = pmi_wr_groups,
};

int sl_sysfs_pmi_wr_create(struct sl_ctrl_lgrp *ctrl_lgrp, struct kobject *parent_kobj)
{
	int rtn;

	rtn = kobject_init_and_add(&ctrl_lgrp->pmi_wr_kobj, &pmi_wr_info, parent_kobj, "wr");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr create kobject_init_and_add failed [%d]", rtn);
		goto out_pmi_wr;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr create (pmi_wr_kobj = 0x%p)", &ctrl_lgrp->pmi_wr_kobj);
	return 0;

out_pmi_wr:
	kobject_put(&ctrl_lgrp->pmi_wr_kobj);

	return -ENOMEM;
}

void sl_sysfs_pmi_wr_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "pmi wr delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&ctrl_lgrp->pmi_wr_kobj);
}
