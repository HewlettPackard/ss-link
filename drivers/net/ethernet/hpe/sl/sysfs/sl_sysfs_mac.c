// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include <linux/hpe/sl/sl_mac.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_mac.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_str.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t rx_state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 state;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, kobj);

	sl_ctrl_mac_rx_state_get(ctrl_mac->ctrl_lgrp->ctrl_ldev->num, ctrl_mac->ctrl_lgrp->num, ctrl_mac->num, &state);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME,
		"rx state show (mac = 0x%p, state = %u)", ctrl_mac, state);

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_mac_state_str(state));
}

static ssize_t tx_state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 state;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, kobj);

	sl_ctrl_mac_tx_state_get(ctrl_mac->ctrl_lgrp->ctrl_ldev->num, ctrl_mac->ctrl_lgrp->num, ctrl_mac->num, &state);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME,
		"tx state show (mac = 0x%p, state = %u)", ctrl_mac, state);

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_mac_state_str(state));
}

static ssize_t info_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_ctrl_mac *ctrl_mac;
	u64                 info_map;
	char                info_map_str[900];

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, kobj);

	rtn = sl_ctrl_mac_info_map_get(ctrl_mac->ctrl_lgrp->ctrl_ldev->num,
		ctrl_mac->ctrl_lgrp->num, ctrl_mac->num, &info_map);
	if (rtn) {
		sl_log_err(ctrl_mac, LOG_BLOCK, LOG_NAME,
			"info map show sl_ctrl_mac_info_map_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "no_mac\n");
	}

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME,
		"info map show (info_map = 0x%llX %s)", info_map, info_map_str);

	return scnprintf(buf, PAGE_SIZE, "%s\n", info_map_str);
}

// FIXME: add other mac info here

static struct kobj_attribute rx_state = __ATTR_RO(rx_state);
static struct kobj_attribute tx_state = __ATTR_RO(tx_state);
static struct kobj_attribute info_map = __ATTR_RO(info_map);

static struct attribute *mac_attrs[] = {
	&rx_state.attr,
	&tx_state.attr,
	&info_map.attr,
	NULL
};
ATTRIBUTE_GROUPS(mac);

static struct kobj_type mac_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = mac_groups,
};

int sl_sysfs_mac_create(struct sl_ctrl_mac *ctrl_mac)
{
	int rtn;

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac create (num = %u)", ctrl_mac->num);

	if (!ctrl_mac->parent_kobj) {
		sl_log_err(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctrl_mac->kobj, &mac_info, ctrl_mac->parent_kobj, "mac");
	if (rtn) {
		sl_log_err(ctrl_mac, LOG_BLOCK, LOG_NAME,
			"mac create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_mac->kobj);
		return rtn;
	}

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME,
		"mac create (mac_kobj = 0x%p)", &ctrl_mac->kobj);

	return 0;
}

void sl_sysfs_mac_delete(struct sl_ctrl_mac *ctrl_mac)
{
	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "delete (num = %u)", ctrl_mac->num);

	if (!ctrl_mac->parent_kobj)
		return;

	kobject_put(&ctrl_mac->kobj);
}
