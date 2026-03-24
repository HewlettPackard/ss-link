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

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rst_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rst_dev_addr_get(core_lgrp, &dev_addr);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rst dev addr show (dev_addr = 0x%02X)", dev_addr);

	return scnprintf(buf, PAGE_SIZE, "0x%02X\n", dev_addr);
}

static ssize_t result_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	struct sl_core_lgrp *core_lgrp;
	int                  result;
	int                  rtn;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, sbus_rst_kobj);
	core_lgrp = sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num);

	rtn = sl_core_lgrp_sbus_rst_result_get(core_lgrp, &result);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "sbus rst result show (result = %d)", result);

	return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static struct kobj_attribute dev_addr = __ATTR_RO(dev_addr);
static struct kobj_attribute result   = __ATTR_RO(result);

static struct attribute *sbus_rst_attrs[] = {
	&dev_addr.attr,
	&result.attr,
	NULL,
};
ATTRIBUTE_GROUPS(sbus_rst);

static struct kobj_type sbus_rst_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = sbus_rst_groups,
};

int sl_sysfs_sbus_rst_create(struct sl_ctrl_lgrp *ctrl_lgrp, struct kobject *parent_kobj)
{
	int rtn;

	rtn = kobject_init_and_add(&ctrl_lgrp->sbus_rst_kobj, &sbus_rst_info, parent_kobj, "rst");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rst create kobject_init_and_add failed [%d]", rtn);
		goto out_sbus_rst;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rst create (sbus_rst_kobj = 0x%p)", &ctrl_lgrp->sbus_rst_kobj);
	return 0;

out_sbus_rst:
	kobject_put(&ctrl_lgrp->sbus_rst_kobj);

	return -ENOMEM;
}

void sl_sysfs_sbus_rst_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "sbus rst delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&ctrl_lgrp->sbus_rst_kobj);
}
