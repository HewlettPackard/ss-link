// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl.h"
#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_ldev.h"
#include "sl_module.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t mod_ver_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_ldev *ctl_ldev;

	ctl_ldev = container_of(kobj, struct sl_ctl_ldev, sl_info_kobj);

	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME,
		"mod ver show (ldev = 0x%p, ver = v%s)", ctl_ldev, sl_version_str_get());

	return scnprintf(buf, PAGE_SIZE, "v%s\n", sl_version_str_get());
}

static ssize_t mod_hash_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_ldev *ctl_ldev;

	ctl_ldev = container_of(kobj, struct sl_ctl_ldev, sl_info_kobj);

	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME,
		"mod hash show (ldev = 0x%p, hash = %s)", ctl_ldev, sl_git_hash_str_get());

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_git_hash_str_get());
}

static struct kobj_attribute mod_ver  = __ATTR_RO(mod_ver);
static struct kobj_attribute mod_hash = __ATTR_RO(mod_hash);

static struct attribute *ldev_attrs[] = {
	&mod_ver.attr,
	&mod_hash.attr,
	NULL
};
ATTRIBUTE_GROUPS(ldev);

static struct kobj_type sl_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = ldev_groups,
};

int sl_sysfs_ldev_create(struct sl_ctl_ldev *ctl_ldev)
{
	int rtn;

	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME, "ldev create (ldev = 0x%p)", ctl_ldev);

	if (!ctl_ldev->parent_kobj) {
		sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "ldev create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&(ctl_ldev->sl_info_kobj), &sl_info, ctl_ldev->parent_kobj, "sl_info");
	if (rtn) {
		sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "sl_info create failed [%d]", rtn);
		kobject_put(&(ctl_ldev->sl_info_kobj));
		return -ENOMEM;
	}

	return 0;
}

void sl_sysfs_ldev_delete(struct sl_ctl_ldev *ctl_ldev)
{
	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME, "ldev delete (ldev = 0x%p)", ctl_ldev);

	if (!ctl_ldev->parent_kobj)
		return;

	kobject_put(&(ctl_ldev->sl_info_kobj));
}
