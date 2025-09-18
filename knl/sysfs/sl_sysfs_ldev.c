// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include <linux/sl_media.h>

#include "sl.h"
#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_ldev.h"
#include "sl_module.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static const char *sl_cable_type_str(u32 type)
{
	switch (type) {
	case SL_CABLE_TYPE_PEC:
		return "PEC";
	case SL_CABLE_TYPE_AOC:
		return "AOC";
	case SL_CABLE_TYPE_POC:
		return "POC";
	case SL_CABLE_TYPE_AEC:
		return "AEC";
	default:
		return "invalid";
	}
}

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

static struct attribute *supported_cables_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(supported_cables);

static struct kobj_type supported_cables_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = supported_cables_groups,
};

static struct attribute *cable_types_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(cable_types);

static struct kobj_type cable_types_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = cable_types_groups,
};

static struct attribute *cable_vendors_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(cable_vendors);

static struct kobj_type cable_vendors_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = cable_vendors_groups,
};

static ssize_t length_cm_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_ldev_cable_hpe_pn_kobj *hpe_pn_kobj;

	hpe_pn_kobj = container_of(kobj, struct sl_ctl_ldev_cable_hpe_pn_kobj, kobj);

	sl_log_dbg(hpe_pn_kobj->ctl_ldev, LOG_BLOCK, LOG_NAME, "length_cm show (ctl_ldev = 0x%p, length_cm = %u)",
		hpe_pn_kobj->ctl_ldev, cable_db[hpe_pn_kobj->cable_idx].length_cm);

	return scnprintf(buf, PAGE_SIZE, "%u\n", cable_db[hpe_pn_kobj->cable_idx].length_cm);
}

static ssize_t max_speed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_ldev_cable_hpe_pn_kobj *hpe_pn_kobj;

	hpe_pn_kobj = container_of(kobj, struct sl_ctl_ldev_cable_hpe_pn_kobj, kobj);

	sl_log_dbg(hpe_pn_kobj->ctl_ldev, LOG_BLOCK, LOG_NAME, "max_speed show (ctl_ldev = 0x%p, max_speed = %s)",
		hpe_pn_kobj->ctl_ldev, sl_media_speed_str(cable_db[hpe_pn_kobj->cable_idx].max_speed));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_speed_str(cable_db[hpe_pn_kobj->cable_idx].max_speed));
}

static struct kobj_attribute cable_hpe_pn_length    = __ATTR_RO(length_cm);
static struct kobj_attribute cable_hpe_pn_max_speed = __ATTR_RO(max_speed);

static struct attribute *cable_hpe_pn_attrs[] = {
	&cable_hpe_pn_length.attr,
	&cable_hpe_pn_max_speed.attr,
	NULL,
};
ATTRIBUTE_GROUPS(cable_hpe_pn);

static struct kobj_type cable_hpe_pns_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = cable_hpe_pn_groups,
};

static void sl_sysfs_cable_types_delete(struct sl_ctl_ldev *ctl_ldev, int type_idx)
{
	int k;

	for (k = 0; k <= type_idx; ++k)
		kobject_put(&ctl_ldev->cable_types_kobj[k]);
}

static int sl_sysfs_cable_types_create(struct sl_ctl_ldev *ctl_ldev)
{
	int type_idx;
	int rtn;

	for (type_idx = SL_CABLE_TYPE_PEC; type_idx < SL_CABLE_TYPES_NUM; ++type_idx) {
		rtn = kobject_init_and_add(&ctl_ldev->cable_types_kobj[type_idx], &cable_types_info,
				&ctl_ldev->supported_cables_kobj, sl_cable_type_str(type_idx));
		if (rtn) {
			sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "type create failed [%d]", rtn);
			sl_sysfs_cable_types_delete(ctl_ldev, type_idx);
			kobject_put(&ctl_ldev->supported_cables_kobj);
			kobject_put(&ctl_ldev->sl_info_kobj);
			return -ENOMEM;
		}
	}

	return 0;
}

static void sl_sysfs_cable_vendors_delete(struct sl_ctl_ldev *ctl_ldev, int type_idx, int vendor_idx)
{
	int k;
	int l;

	for (k = 0; k <= type_idx; ++k) {
		for (l = 0; l <= vendor_idx; ++l)
			kobject_put(&ctl_ldev->cable_vendors_kobj[k][l]);
	}
}

static int sl_sysfs_cable_vendors_create(struct sl_ctl_ldev *ctl_ldev)
{
	int type_idx;
	int vendor_idx;
	int rtn;

	for (type_idx = SL_CABLE_TYPE_PEC; type_idx < SL_CABLE_TYPES_NUM; ++type_idx) {
		for (vendor_idx = 0; vendor_idx < SL_CABLE_VENDORS_NUM; ++vendor_idx) {
			rtn = kobject_init_and_add(&ctl_ldev->cable_vendors_kobj[type_idx][vendor_idx], &cable_vendors_info,
					&ctl_ldev->cable_types_kobj[type_idx], sl_media_vendor_str(vendor_idx + 1));
			if (rtn) {
				sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "vendor create failed [%d]", rtn);
				sl_sysfs_cable_vendors_delete(ctl_ldev, type_idx, vendor_idx);
				sl_sysfs_cable_types_delete(ctl_ldev, type_idx);
				kobject_put(&ctl_ldev->supported_cables_kobj);
				kobject_put(&ctl_ldev->sl_info_kobj);
				return -ENOMEM;
			}
		}
	}
	return 0;
}

static void sl_sysfs_cable_db_delete(struct sl_ctl_ldev *ctl_ldev, int db_idx)
{
	int k;

	for (k = 0; k <= db_idx; ++k)
		kobject_put(&ctl_ldev->cable_hpe_pns_kobj[k].kobj);
}

static int sl_sysfs_cable_db_create(struct sl_ctl_ldev *ctl_ldev)
{
	char hpe_pn[12];
	int  i;
	int  type_kobj_num;
	int  rtn;

	for (i = 0; i < ARRAY_SIZE(cable_db); ++i) {
		switch (cable_db[i].type) {
		case SL_MEDIA_TYPE_PEC:
			type_kobj_num = SL_CABLE_TYPE_PEC;
			break;
		case SL_MEDIA_TYPE_AOC:
			type_kobj_num = SL_CABLE_TYPE_AOC;
			break;
		case SL_MEDIA_TYPE_POC:
			type_kobj_num = SL_CABLE_TYPE_POC;
			break;
		case SL_MEDIA_TYPE_AEC:
			type_kobj_num = SL_CABLE_TYPE_AEC;
			break;
		default:
			sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME,
				"unknown (type = %u)", cable_db[i].type);
			continue;
		}
		ctl_ldev->cable_hpe_pns_kobj[i].ctl_ldev = ctl_ldev;
		ctl_ldev->cable_hpe_pns_kobj[i].cable_idx = i;
		snprintf(hpe_pn, sizeof(hpe_pn), "%u", cable_db[i].hpe_pn);
		rtn = kobject_init_and_add(&ctl_ldev->cable_hpe_pns_kobj[i].kobj, &cable_hpe_pns_info,
				&ctl_ldev->cable_vendors_kobj[type_kobj_num][cable_db[i].vendor - 1], hpe_pn);
		if (rtn) {
			sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "hpe_pn create failed [%d]", rtn);
			sl_sysfs_cable_db_delete(ctl_ldev, i);
			sl_sysfs_cable_vendors_delete(ctl_ldev, SL_CABLE_TYPES_NUM - 1, SL_CABLE_VENDORS_NUM - 1);
			sl_sysfs_cable_types_delete(ctl_ldev, SL_CABLE_TYPES_NUM - 1);
			kobject_put(&ctl_ldev->supported_cables_kobj);
			kobject_put(&ctl_ldev->sl_info_kobj);
			return -ENOMEM;
		}
	}

	return 0;
}

static int sl_sysfs_cable_info_create(struct sl_ctl_ldev *ctl_ldev)
{
	int  rtn;

	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME, "cable info create (ldev = 0x%p)", ctl_ldev);

	rtn = kobject_init_and_add(&ctl_ldev->supported_cables_kobj, &supported_cables_info,
			ctl_ldev->parent_kobj, "supported_cables");
	if (rtn) {
		sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "supported cables create failed [%d]", rtn);
		kobject_put(&ctl_ldev->supported_cables_kobj);
		kobject_put(&ctl_ldev->sl_info_kobj);
		return -ENOMEM;
	}

	rtn = sl_sysfs_cable_types_create(ctl_ldev);
	if (rtn) {
		sl_log_err_trace(ctl_ldev, LOG_BLOCK, LOG_NAME, "cable types create failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_sysfs_cable_vendors_create(ctl_ldev);
	if (rtn) {
		sl_log_err_trace(ctl_ldev, LOG_BLOCK, LOG_NAME, "cable vendors create failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_sysfs_cable_db_create(ctl_ldev);
	if (rtn) {
		sl_log_err_trace(ctl_ldev, LOG_BLOCK, LOG_NAME, "cable db create failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_sysfs_ldev_create(u8 ldev_num, struct kobject *parent)
{
	int                 rtn;
	struct sl_ctl_ldev *ctl_ldev;

	ctl_ldev = sl_ctl_ldev_get(ldev_num);
	if (!ctl_ldev) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sysfs create ldev not found (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	if (!sl_ctl_ldev_kref_get_unless_zero(ctl_ldev)) {
		sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME,
			"kref_get_unless_zero failed (ctl_ldev = 0x%p)", ctl_ldev);
		return -EBADRQC;
	}

	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME, "ldev create (ldev = 0x%p)", ctl_ldev);

	ctl_ldev->parent_kobj = parent;

	rtn = kobject_init_and_add(&(ctl_ldev->sl_info_kobj), &sl_info, ctl_ldev->parent_kobj, "sl_info");
	if (rtn) {
		sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "sl_info create failed [%d]", rtn);
		kobject_put(&(ctl_ldev->sl_info_kobj));
		rtn = -ENOMEM;
		goto out;
	}

	rtn = sl_sysfs_cable_info_create(ctl_ldev);
	if (rtn) {
		sl_log_err(ctl_ldev, LOG_BLOCK, LOG_NAME, "cable info create failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:
	if (sl_ctl_ldev_put(ctl_ldev))
		sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME, "ldev create - ldev removed (ldev = 0x%p)", ctl_ldev);

	return 0;
}

static void sl_sysfs_cable_info_delete(struct sl_ctl_ldev *ctl_ldev)
{
	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME, "cable info delete (ldev = 0x%p)", ctl_ldev);

	sl_sysfs_cable_db_delete(ctl_ldev, ARRAY_SIZE(cable_db) - 1);
	sl_sysfs_cable_vendors_delete(ctl_ldev, SL_CABLE_TYPES_NUM - 1, SL_CABLE_VENDORS_NUM - 1);
	sl_sysfs_cable_types_delete(ctl_ldev, SL_CABLE_TYPES_NUM - 1);

	kobject_put(&ctl_ldev->supported_cables_kobj);
}

void sl_sysfs_ldev_delete(struct sl_ctl_ldev *ctl_ldev)
{
	sl_log_dbg(ctl_ldev, LOG_BLOCK, LOG_NAME, "ldev delete (ldev = 0x%p)", ctl_ldev);

	if (!ctl_ldev->parent_kobj)
		return;

	sl_sysfs_cable_info_delete(ctl_ldev);
	kobject_put(&(ctl_ldev->sl_info_kobj));
}
