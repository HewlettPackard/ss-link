// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include <linux/hpe/sl/sl_ldev.h>

#include "sl_log.h"
#include "sl_ldev.h"
#include "sl_media_ldev.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_LDEV_LOG_NAME

int sl_ldev_attr_check(struct sl_ldev_attr *ldev_attr)
{
	if (!ldev_attr) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ldev_attr");
		return -EINVAL;
	}
	if (IS_ERR(ldev_attr)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "ldev_attr pointer error");
		return -EINVAL;
	}
	if (ldev_attr->magic != SL_LDEV_ATTR_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev_attr magic");
		return -EINVAL;
	}
	if (ldev_attr->ver != SL_LDEV_ATTR_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev_attr version");
		return -EINVAL;
	}

	if (!ldev_attr->ops->read64) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops read64");
		return -EINVAL;
	}
	if (!ldev_attr->ops->write64) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops write64");
		return -EINVAL;
	}
	if (!ldev_attr->ops->sbus_op) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops sbus_op");
		return -EINVAL;
	}
	if (!ldev_attr->ops->pmi_op) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops pmi_op");
		return -EINVAL;
	}
	if (!ldev_attr->ops->pml_intr_register) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops pml_intr_register");
		return -EINVAL;
	}
	if (!ldev_attr->ops->pml_intr_unregister) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops pml_intr_unregister");
		return -EINVAL;
	}
	if (!ldev_attr->ops->pml_intr_enable) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops pml_intr_enable");
		return -EINVAL;
	}
	if (!ldev_attr->ops->pml_intr_disable) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops pml_intr_disable");
		return -EINVAL;
	}
	if (!ldev_attr->ops->dt_info_get) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops dt_info_get");
		return -EINVAL;
	}
	if (!ldev_attr->ops->mb_info_get) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops mb_info_get");
		return -EINVAL;
	}
	if (!ldev_attr->ops->dmac_alloc) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops dmac_alloc");
		return -EINVAL;
	}
	if (!ldev_attr->ops->dmac_free) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops dmac_free");
		return -EINVAL;
	}
	if (!ldev_attr->ops->dmac_xfer) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ops dmac_xfer");
		return -EINVAL;
	}

	if (!ldev_attr->accessors->pci) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor PCI");
		return -EINVAL;
	}
	if (!ldev_attr->accessors->sbus) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor SBUS");
		return -EINVAL;
	}
	if (!ldev_attr->accessors->pmi) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor PMI");
		return -EINVAL;
	}
	if (!ldev_attr->accessors->intr) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor INTR");
		return -EINVAL;
	}
	if (!ldev_attr->accessors->dt) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor DT");
		return -EINVAL;
	}
	if (!ldev_attr->accessors->mb) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor MB");
		return -EINVAL;
	}
	if (!ldev_attr->accessors->dmac) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor DMAC");
		return -EINVAL;
	}

	return 0;
}

int sl_ldev_uc_ops_set(struct sl_ldev *ldev, struct sl_uc_ops *uc_ops,
		       struct sl_uc_accessor *uc_accessor)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "check fail");
		return rtn;
	}

	sl_log_dbg(ldev, LOG_BLOCK, LOG_NAME, "uc ops_set");

	rtn = sl_media_ldev_uc_ops_set(ldev->num, uc_ops, uc_accessor);
	if (rtn) {
		sl_log_err(ldev, LOG_BLOCK, LOG_NAME, "media_ldev_uc_ops_set failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
EXPORT_SYMBOL(sl_ldev_uc_ops_set);
