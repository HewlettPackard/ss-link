// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl.h"
#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_ldev.h"
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
		return "unknown";
	}
}

static ssize_t mod_ver_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_ldev *ctrl_ldev;

	ctrl_ldev = container_of(kobj, struct sl_ctrl_ldev, sl_info_kobj);

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME,
		   "mod ver show (ldev = 0x%p, ver = v%s)", ctrl_ldev, sl_version_str_get());

	return scnprintf(buf, PAGE_SIZE, "v%s\n", sl_version_str_get());
}

static ssize_t mod_hash_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_ldev *ctrl_ldev;

	ctrl_ldev = container_of(kobj, struct sl_ctrl_ldev, sl_info_kobj);

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME,
		   "mod hash show (ldev = 0x%p, hash = %s)", ctrl_ldev, sl_git_hash_str_get());

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
	struct sl_ctrl_ldev_cable_hpe_pn_kobj *hpe_pn_kobj;

	hpe_pn_kobj = container_of(kobj, struct sl_ctrl_ldev_cable_hpe_pn_kobj, kobj);

	sl_log_dbg(hpe_pn_kobj->ctrl_ldev, LOG_BLOCK, LOG_NAME,
		   "length_cm show (ctrl_ldev = 0x%p, length_cm = %u)",
		   hpe_pn_kobj->ctrl_ldev, cable_db[hpe_pn_kobj->cable_idx].length_cm);

	return scnprintf(buf, PAGE_SIZE, "%u\n", cable_db[hpe_pn_kobj->cable_idx].length_cm);
}

static ssize_t max_speed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_ldev_cable_hpe_pn_kobj *hpe_pn_kobj;

	hpe_pn_kobj = container_of(kobj, struct sl_ctrl_ldev_cable_hpe_pn_kobj, kobj);

	sl_log_dbg(hpe_pn_kobj->ctrl_ldev, LOG_BLOCK, LOG_NAME,
		   "max_speed show (ctrl_ldev = 0x%p, max_speed = %s)",
		   hpe_pn_kobj->ctrl_ldev, sl_media_speed_str(cable_db[hpe_pn_kobj->cable_idx].max_speed));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_media_speed_str(cable_db[hpe_pn_kobj->cable_idx].max_speed));
}

static struct kobj_attribute cable_hpe_pn_length_cm = __ATTR_RO(length_cm);
static struct kobj_attribute cable_hpe_pn_max_speed = __ATTR_RO(max_speed);

static struct attribute *cable_hpe_pn_attrs[] = {
	&cable_hpe_pn_length_cm.attr,
	&cable_hpe_pn_max_speed.attr,
	NULL,
};
ATTRIBUTE_GROUPS(cable_hpe_pn);

static struct kobj_type cable_hpe_pns_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = cable_hpe_pn_groups,
};

static void sl_sysfs_cable_types_delete(struct sl_ctrl_ldev *ctrl_ldev, int type_idx)
{
	int k;

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "types delete");

	for (k = 0; k < type_idx; ++k) {
		sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "types delete (type = %d)", k);
		kobject_put(&ctrl_ldev->cable_types_kobj[k]);
	}
}

static int sl_sysfs_cable_types_create(struct sl_ctrl_ldev *ctrl_ldev)
{
	int type_idx;
	int rtn;

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "types create");

	for (type_idx = 0; type_idx < SL_CABLE_TYPES_NUM; ++type_idx) {
		sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "types create (type = %d)", type_idx);
		rtn = kobject_init_and_add(&ctrl_ldev->cable_types_kobj[type_idx],
					   &cable_types_info, &ctrl_ldev->supported_cables_kobj,
					   sl_cable_type_str(type_idx));
		if (rtn) {
			sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME,
				   "types create failed (type = %d) [%d]", type_idx, rtn);
			kobject_put(&ctrl_ldev->cable_types_kobj[type_idx]);
			sl_sysfs_cable_types_delete(ctrl_ldev, type_idx);
			return -ENOMEM;
		}
	}

	return 0;
}

static void sl_sysfs_cable_vendors_delete(struct sl_ctrl_ldev *ctrl_ldev, int type_idx, int vendor_idx)
{
	int k;
	int l;

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "vendors delete");

	for (k = 0; k < type_idx; ++k) {
		for (l = 0; l < vendor_idx; ++l) {
			sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME,
				   "vendors delete (type = %d, vendor = %d)", k, l);
			kobject_put(&ctrl_ldev->cable_vendors_kobj[k][l]);
		}
	}
}

static int sl_sysfs_cable_vendors_create(struct sl_ctrl_ldev *ctrl_ldev)
{
	int type_idx;
	int vendor_idx;
	int rtn;
	int x;

	BUILD_BUG_ON((SL_MEDIA_VENDOR_AMPHENOL > 10) || (SL_CABLE_VENDORS_NUM > 10));

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "vendors create");

	for (type_idx = 0; type_idx < SL_CABLE_TYPES_NUM; ++type_idx) {
		for (vendor_idx = 0; vendor_idx < SL_CABLE_VENDORS_NUM; ++vendor_idx) {
			sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME,
				   "vendors create (type = %d, vendor = %d)", type_idx, vendor_idx);
			rtn = kobject_init_and_add(&ctrl_ldev->cable_vendors_kobj[type_idx][vendor_idx],
						   &cable_vendors_info, &ctrl_ldev->cable_types_kobj[type_idx],
						   sl_media_vendor_str(vendor_idx + SL_MEDIA_VENDOR_TE));
			if (rtn) {
				sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME,
					   "vendors create failed (type = %d, vendor = %d) [%d]",
					   type_idx, vendor_idx, rtn);
				kobject_put(&ctrl_ldev->cable_vendors_kobj[type_idx][vendor_idx]);
				for (x = 0; x < vendor_idx; ++x) {
					sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME,
						   "vendors create delete (type = %d, vendor = %d)", type_idx, x);
					kobject_put(&ctrl_ldev->cable_vendors_kobj[type_idx][x]);
				}
				sl_sysfs_cable_vendors_delete(ctrl_ldev, type_idx, SL_CABLE_VENDORS_NUM);
				sl_sysfs_cable_types_delete(ctrl_ldev, SL_CABLE_TYPES_NUM);
				return -ENOMEM;
			}
		}
	}

	return 0;
}

static void sl_sysfs_cable_db_delete(struct sl_ctrl_ldev *ctrl_ldev, int db_idx)
{
	int k;

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "db delete");

	for (k = 0; k < db_idx; ++k) {
		sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "db delete (idx = %d)", k);
		kobject_put(&ctrl_ldev->cable_hpe_pns_kobj[k].kobj);
	}
}

static int sl_sysfs_cable_db_create(struct sl_ctrl_ldev *ctrl_ldev)
{
	char hpe_pn[SL_MEDIA_HPE_PN_SIZE + SL_MEDIA_VENDOR_PN_SIZE];
	int  i;
	int  type_kobj_num;
	int  rtn;

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "db create");

	for (i = 0; i < ARRAY_SIZE(cable_db); ++i) {
		sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "db create (idx = %d)", i);
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
			sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME,
				   "unknown (type = %u)", cable_db[i].type);
			continue;
		}
		ctrl_ldev->cable_hpe_pns_kobj[i].ctrl_ldev = ctrl_ldev;
		ctrl_ldev->cable_hpe_pns_kobj[i].cable_idx = i;
		snprintf(hpe_pn, sizeof(hpe_pn), "%u_%s", cable_db[i].hpe_pn, cable_db[i].vendor_pn);
		rtn = kobject_init_and_add(&ctrl_ldev->cable_hpe_pns_kobj[i].kobj, &cable_hpe_pns_info,
					   &ctrl_ldev->cable_vendors_kobj[type_kobj_num][cable_db[i].vendor - SL_MEDIA_VENDOR_TE],
					   hpe_pn);
		if (rtn) {
			sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME,
				   "hpe_pn create failed (idx = %d, type = 0x%X %s, vendor = %u %s, len = %ucm, hpe_pn = %u) [%d]",
				   i, cable_db[i].type, sl_media_type_str(cable_db[i].type),
				   cable_db[i].vendor, sl_media_vendor_str(cable_db[i].vendor),
				   cable_db[i].length_cm, cable_db[i].hpe_pn, rtn);
			kobject_put(&ctrl_ldev->cable_hpe_pns_kobj[i].kobj);
			sl_sysfs_cable_db_delete(ctrl_ldev, i);
			sl_sysfs_cable_vendors_delete(ctrl_ldev, SL_CABLE_TYPES_NUM, SL_CABLE_VENDORS_NUM);
			sl_sysfs_cable_types_delete(ctrl_ldev, SL_CABLE_TYPES_NUM);
			return -ENOMEM;
		}
	}

	return 0;
}

static int sl_sysfs_cable_info_create(struct sl_ctrl_ldev *ctrl_ldev)
{
	int rtn;

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "cable info create (ldev = 0x%p)", ctrl_ldev);

	rtn = kobject_init_and_add(&ctrl_ldev->supported_cables_kobj, &supported_cables_info,
			ctrl_ldev->parent_kobj, "supported_cables");
	if (rtn) {
		sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME, "supported cables create failed [%d]", rtn);
		kobject_put(&ctrl_ldev->supported_cables_kobj);
		return -ENOMEM;
	}

	rtn = sl_sysfs_cable_types_create(ctrl_ldev);
	if (rtn) {
		sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME, "cable types create failed [%d]", rtn);
		kobject_put(&ctrl_ldev->supported_cables_kobj);
		return rtn;
	}

	rtn = sl_sysfs_cable_vendors_create(ctrl_ldev);
	if (rtn) {
		sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME, "cable vendors create failed [%d]", rtn);
		kobject_put(&ctrl_ldev->supported_cables_kobj);
		return rtn;
	}

	rtn = sl_sysfs_cable_db_create(ctrl_ldev);
	if (rtn) {
		sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME, "cable db create failed [%d]", rtn);
		kobject_put(&ctrl_ldev->supported_cables_kobj);
		return rtn;
	}

	return 0;
}

int sl_sysfs_ldev_create(u8 ldev_num, struct kobject *parent)
{
	int                 rtn;
	struct sl_ctrl_ldev *ctrl_ldev;

	ctrl_ldev = sl_ctrl_ldev_get(ldev_num);
	if (!ctrl_ldev) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sysfs create ldev not found (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_ldev_kref_get_unless_zero(ctrl_ldev)) {
		sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME,
			"kref_get_unless_zero failed (ldev = 0x%p)", ctrl_ldev);
		return -EBADRQC;
	}

	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "ldev create (ldev = 0x%p)", ctrl_ldev);

	if (ctrl_ldev->is_sysfs_ok)
		return 0;

	ctrl_ldev->parent_kobj = parent;
	ctrl_ldev->is_sysfs_ok = false;

	rtn = kobject_init_and_add(&(ctrl_ldev->sl_info_kobj), &sl_info, ctrl_ldev->parent_kobj, "sl_info");
	if (rtn) {
		sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME, "sl_info create failed [%d]", rtn);
		kobject_put(&ctrl_ldev->sl_info_kobj);
		rtn = -ENOMEM;
		goto out;
	}

	rtn = sl_sysfs_cable_info_create(ctrl_ldev);
	if (rtn) {
		sl_log_err(ctrl_ldev, LOG_BLOCK, LOG_NAME, "cable info create failed [%d]", rtn);
		kobject_put(&ctrl_ldev->sl_info_kobj);
		goto out;
	}

	ctrl_ldev->is_sysfs_ok = true;
	rtn = 0;

out:
	if (sl_ctrl_ldev_put(ctrl_ldev))
		sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "ldev create removed (ldev = 0x%p)", ctrl_ldev);

	return rtn;
}

void sl_sysfs_ldev_delete(struct sl_ctrl_ldev *ctrl_ldev)
{
	sl_log_dbg(ctrl_ldev, LOG_BLOCK, LOG_NAME, "ldev delete (ldev = 0x%p)", ctrl_ldev);

	if (!ctrl_ldev->parent_kobj)
		return;
	if (!ctrl_ldev->is_sysfs_ok)
		return;

	sl_sysfs_cable_db_delete(ctrl_ldev, ARRAY_SIZE(cable_db));
	sl_sysfs_cable_vendors_delete(ctrl_ldev, SL_CABLE_TYPES_NUM, SL_CABLE_VENDORS_NUM);
	sl_sysfs_cable_types_delete(ctrl_ldev, SL_CABLE_TYPES_NUM);

	kobject_put(&ctrl_ldev->supported_cables_kobj);
	kobject_put(&ctrl_ldev->sl_info_kobj);
}
