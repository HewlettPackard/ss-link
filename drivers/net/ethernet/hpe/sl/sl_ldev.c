// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/err.h>

#include <linux/hpe/sl/sl_ldev.h>

#include "sl_asic.h"
#include "sl_log.h"
#include "sl_ldev.h"
#include "sl_sysfs.h"
#include "sl_ctrl_ldev.h"
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

struct sl_ldev *sl_ldev_new(u8 ldev_num,
	struct workqueue_struct *workq, struct sl_ldev_attr *ldev_attr)
{
	int rtn;

	if (ldev_num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "invalid (ldev_num = %u)", ldev_num);
		return ERR_PTR(-EINVAL);
	}
	rtn = sl_ldev_attr_check(ldev_attr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
		return ERR_PTR(rtn);
	}

	rtn = sl_ctrl_ldev_new(ldev_num, workq, ldev_attr);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
		return ERR_PTR(rtn);
	}

	return &ldevs[ldev_num];
}
EXPORT_SYMBOL(sl_ldev_new);

int sl_ldev_sysfs_parent_set(struct sl_ldev *ldev, struct kobject *parent)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "parent set fail");
		return rtn;
	}
	if (!parent) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL parent");
		return -EINVAL;
	}

	return sl_sysfs_ldev_create(ldev->num, parent);
}
EXPORT_SYMBOL(sl_ldev_sysfs_parent_set);

int sl_ldev_serdes_init(struct sl_ldev *ldev)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "serdes init fail");
		return rtn;
	}

	return sl_ctrl_ldev_serdes_init(ldev->num);
}
EXPORT_SYMBOL(sl_ldev_serdes_init);

int sl_ldev_del(struct sl_ldev *ldev)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "del fail");
		return rtn;
	}

	return sl_ctrl_ldev_del(ldev->num);
}
EXPORT_SYMBOL(sl_ldev_del);

void sl_ldev_exit(void)
{
	sl_ctrl_ldev_exit();
}
