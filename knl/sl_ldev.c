// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/err.h>

#include <linux/sl_ldev.h>

#include "sl_asic.h"
#include "sl_log.h"
#include "sl_ldev.h"
#include "sl_sysfs.h"
#include "sl_ctl_ldev.h"
#include "sl_media_ldev.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_LDEV_LOG_NAME

static struct sl_ldev ldevs[SL_ASIC_MAX_LDEVS];

void sl_ldev_init(void)
{
	u8 ldev_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		ldevs[ldev_num].magic = SL_LDEV_MAGIC;
		ldevs[ldev_num].ver   = SL_LDEV_VER;
		ldevs[ldev_num].size  = sizeof(struct sl_ldev);
		ldevs[ldev_num].num   = ldev_num;
	}
}

int sl_ldev_check(struct sl_ldev *ldev)
{
	if (!ldev) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ldev");
		return -EINVAL;
	}
	if (IS_ERR(ldev)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "ldev pointer error");
		return -EINVAL;
	}
	if (ldev->magic != SL_LDEV_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev magic");
		return -EINVAL;
	}
	if (ldev->ver != SL_LDEV_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev version");
		return -EINVAL;
	}
	if (ldev->num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev num");
		return -EINVAL;
	}

	return 0;
}

static int sl_ldev_attr_check(struct sl_ldev_attr *ldev_attr)
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

	/* Optional operations for use on systems supporting DMAC */
	if (!ldev_attr->ops->dmac_alloc)
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "NULL ops dmac_alloc");
	if (!ldev_attr->ops->dmac_free)
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "NULL ops dmac_free");
	if (!ldev_attr->ops->dmac_xfer)
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "NULL ops dmac_xfer");

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

	/* Optional accessors for use on systems supporting DMAC */
	if (!ldev_attr->accessors->dmac)
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "NULL accessor DMAC");

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
#ifdef BUILDSYS_FRAMEWORK_CASSINI
EXPORT_SYMBOL(sl_ldev_uc_ops_set);
#endif /* BUILDSYS_FRAMEWORK_CASSINI */

struct sl_ldev *sl_ldev_new(u8 ldev_num,
	struct workqueue_struct *workq, struct sl_ldev_attr *ldev_attr)
{
	int rtn;

	if (ldev_num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"invalid (ldev_num = %u)", ldev_num);
		return ERR_PTR(-EINVAL);
	}
	rtn = sl_ldev_attr_check(ldev_attr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "check fail");
		return ERR_PTR(rtn);
	}

	rtn = sl_ctl_ldev_new(ldev_num, workq, ldev_attr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
		return ERR_PTR(rtn);
	}

	return &ldevs[ldev_num];
}
EXPORT_SYMBOL(sl_ldev_new);

int sl_ldev_sysfs_parent_set(struct sl_ldev *ldev, struct kobject *parent)
{
	int                 rtn;
	struct sl_ctl_ldev *ctl_ldev;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "check fail");
		return rtn;
	}
	if (!parent) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL parent");
		return -EINVAL;
	}

	ctl_ldev = sl_ctl_ldev_get(ldev->num);
	if (!ctl_ldev) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ldev");
		return -ENOMEM;
	}

	if (ctl_ldev->parent_kobj) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "parent already set");
		return 0;
	}

	ctl_ldev->parent_kobj = parent;

	rtn = sl_sysfs_ldev_create(ctl_ldev);
	if (rtn) {
		sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME,
			"sysfs_ldev_create failed");
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL(sl_ldev_sysfs_parent_set);

int sl_ldev_serdes_init(struct sl_ldev *ldev)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "check fail");
		return rtn;
	}

	return sl_ctl_ldev_serdes_init(ldev->num);
}
EXPORT_SYMBOL(sl_ldev_serdes_init);

int sl_ldev_del(struct sl_ldev *ldev)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "check fail");
		return rtn;
	}

	sl_ctl_ldev_del(ldev->num);

	return 0;
}
EXPORT_SYMBOL(sl_ldev_del);

void sl_ldev_exit(void)
{
	sl_ctl_ldev_exit();
}
