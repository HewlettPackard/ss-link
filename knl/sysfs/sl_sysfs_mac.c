// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include <linux/sl_mac.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_mac.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t rx_state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_mac *ctl_mac;
	u32                state;

	ctl_mac = container_of(kobj, struct sl_ctl_mac, kobj);

	sl_ctl_mac_rx_state_get(ctl_mac->ctl_lgrp->ctl_ldev->num, ctl_mac->ctl_lgrp->num, ctl_mac->num, &state);

	sl_log_dbg(ctl_mac, LOG_BLOCK, LOG_NAME,
		"rx state show (mac = 0x%p, state = %u)", ctl_mac, state);

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_mac_state_str(state));
}

static ssize_t tx_state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_mac *ctl_mac;
	u32                state;

	ctl_mac = container_of(kobj, struct sl_ctl_mac, kobj);

	sl_ctl_mac_tx_state_get(ctl_mac->ctl_lgrp->ctl_ldev->num, ctl_mac->ctl_lgrp->num, ctl_mac->num, &state);

	sl_log_dbg(ctl_mac, LOG_BLOCK, LOG_NAME,
		"tx state show (mac = 0x%p, state = %u)", ctl_mac, state);

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_mac_state_str(state));
}

// FIXME: add other mac info here

static struct kobj_attribute rx_state = __ATTR_RO(rx_state);
static struct kobj_attribute tx_state = __ATTR_RO(tx_state);

static struct attribute *mac_attrs[] = {
	&rx_state.attr,
	&tx_state.attr,
	NULL
};
ATTRIBUTE_GROUPS(mac);

static struct kobj_type mac_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = mac_groups,
};

int sl_sysfs_mac_create(struct sl_ctl_mac *ctl_mac)
{
	int rtn;

	sl_log_dbg(ctl_mac, LOG_BLOCK, LOG_NAME, "mac create (num = %u)", ctl_mac->num);

	if (!ctl_mac->parent_kobj) {
		sl_log_err(ctl_mac, LOG_BLOCK, LOG_NAME, "mac create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&ctl_mac->kobj, &mac_info, ctl_mac->parent_kobj, "mac");
	if (rtn) {
		sl_log_err(ctl_mac, LOG_BLOCK, LOG_NAME,
			"mac create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_mac->kobj);
		return rtn;
	}

	sl_log_dbg(ctl_mac, LOG_BLOCK, LOG_NAME,
		"mac create (mac_kobj = 0x%p)", &ctl_mac->kobj);

	return 0;
}

void sl_sysfs_mac_delete(struct sl_ctl_mac *ctl_mac)
{
	sl_log_dbg(ctl_mac, LOG_BLOCK, LOG_NAME, "delete (num = %u)", ctl_mac->num);

	if (!ctl_mac->parent_kobj)
		return;

	kobject_put(&ctl_mac->kobj);
}
