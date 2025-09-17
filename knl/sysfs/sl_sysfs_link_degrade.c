// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/types.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_link.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	struct sl_core_link *core_link;
	int                  degrade_state;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, degrade_kobj);
	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	spin_lock(&core_link->data_lock);
	degrade_state = core_link->degrade_state;
	spin_unlock(&core_link->data_lock);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "degrade state show (state = %u %s)",
		degrade_state, sl_link_degrade_state_str(degrade_state));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_link_degrade_state_str(degrade_state));
}

static ssize_t is_rx_degraded_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	struct sl_core_link *core_link;
	bool                 is_rx_degraded;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, degrade_kobj);
	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	spin_lock(&core_link->data_lock);
	is_rx_degraded = core_link->degrade_info.is_rx_degrade;
	spin_unlock(&core_link->data_lock);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "is rx degraded show (degrade = %u %s)",
		is_rx_degraded, is_rx_degraded ? "yes" : "no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_rx_degraded ? "yes" : "no");
}

static ssize_t rx_lane_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	struct sl_core_link *core_link;
	u8                   lane_map;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, degrade_kobj);
	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	if (!sl_core_link_is_degrade_state_enabled(core_link))
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");

	spin_lock(&core_link->data_lock);
	lane_map = core_link->degrade_info.rx_lane_map;
	spin_unlock(&core_link->data_lock);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"rx degrade lane map show (lane_map = 0x%X)", lane_map);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n", lane_map);
}

static ssize_t rx_link_speed_gbps_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	struct sl_core_link *core_link;
	u16                  link_speed;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, degrade_kobj);
	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	if (!sl_core_link_is_degrade_state_enabled(core_link))
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");

	spin_lock(&core_link->data_lock);
	link_speed = core_link->degrade_info.rx_link_speed;
	spin_unlock(&core_link->data_lock);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"rx degrade link speed gbps show (link_speed = %u)", link_speed);

	return scnprintf(buf, PAGE_SIZE, "%u\n", link_speed);
}

static ssize_t is_tx_degraded_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	struct sl_core_link *core_link;
	bool                 is_tx_degraded;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, degrade_kobj);
	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	spin_lock(&core_link->data_lock);
	is_tx_degraded = core_link->degrade_info.is_tx_degrade;
	spin_unlock(&core_link->data_lock);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "is tx degraded show (degrade = %u %s)",
		is_tx_degraded, is_tx_degraded ? "yes" : "no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_tx_degraded ? "yes" : "no");
}

static ssize_t tx_lane_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	struct sl_core_link *core_link;
	u8                   lane_map;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, degrade_kobj);
	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	if (!sl_core_link_is_degrade_state_enabled(core_link))
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");

	spin_lock(&core_link->data_lock);
	lane_map = core_link->degrade_info.tx_lane_map;
	spin_unlock(&core_link->data_lock);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"tx degrade lane map show (lane_map = 0x%X)", lane_map);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n", lane_map);
}

static ssize_t tx_link_speed_gbps_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	struct sl_core_link *core_link;
	u16                  link_speed;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, degrade_kobj);
	core_link = sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num);

	if (!sl_core_link_is_degrade_state_enabled(core_link))
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");

	spin_lock(&core_link->data_lock);
	link_speed = core_link->degrade_info.tx_link_speed;
	spin_unlock(&core_link->data_lock);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"tx degrade link speed gbps show (link_speed = %u)", link_speed);

	return scnprintf(buf, PAGE_SIZE, "%u\n", link_speed);
}

static struct kobj_attribute link_degrade_state              = __ATTR_RO(state);
static struct kobj_attribute link_is_rx_degraded             = __ATTR_RO(is_rx_degraded);
static struct kobj_attribute link_rx_degrade_lane_map        = __ATTR_RO(rx_lane_map);
static struct kobj_attribute link_rx_degrade_link_speed_gbps = __ATTR_RO(rx_link_speed_gbps);
static struct kobj_attribute link_is_tx_degraded             = __ATTR_RO(is_tx_degraded);
static struct kobj_attribute link_tx_degrade_lane_map        = __ATTR_RO(tx_lane_map);
static struct kobj_attribute link_tx_degrade_link_speed_gbps = __ATTR_RO(tx_link_speed_gbps);

static struct attribute *link_degrade_attrs[] = {
	&link_degrade_state.attr,
	&link_is_rx_degraded.attr,
	&link_rx_degrade_lane_map.attr,
	&link_rx_degrade_link_speed_gbps.attr,
	&link_is_tx_degraded.attr,
	&link_tx_degrade_lane_map.attr,
	&link_tx_degrade_link_speed_gbps.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_degrade);

static struct kobj_type link_degrade = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_degrade_groups,
};

int sl_sysfs_link_degrade_create(struct sl_ctrl_link *ctrl_link)
{
	int rtn;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link degrade create (num = %u)", ctrl_link->num);

	rtn = kobject_init_and_add(&ctrl_link->degrade_kobj, &link_degrade, &ctrl_link->kobj, "auto_lane_degrade");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link degrade create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->degrade_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_degrade_delete(struct sl_ctrl_link *ctrl_link)
{
	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link degrade delete (num = %u)", ctrl_link->num);

	kobject_put(&ctrl_link->degrade_kobj);
}
