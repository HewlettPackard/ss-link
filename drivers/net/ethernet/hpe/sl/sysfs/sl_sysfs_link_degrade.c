// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/types.h>

#include "sl_log.h"
#include "data/sl_core_data_link.h"

#include "sl_sysfs.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	int                  degrade_state;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_degrade_state_get(core_link, &degrade_state);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "degrade state show (state = %u %s)",
		   degrade_state, sl_link_degrade_state_str(degrade_state));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_link_degrade_state_str(degrade_state));
}

static ssize_t is_rx_degraded_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	bool                 is_rx_degraded;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_is_rx_degrade_get(core_link, &is_rx_degraded);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "is rx degraded show (is_rx_degraded = %s)",
		   is_rx_degraded ? "yes" : "no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_rx_degraded ? "yes" : "no");
}

static ssize_t rx_degrade_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	u8                   rx_degrade_map;
	bool                 is_rx_degraded;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_is_rx_degrade_get(core_link, &is_rx_degraded);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	if (!is_rx_degraded)
		return scnprintf(buf, PAGE_SIZE, "no\n");

	rtn = sl_core_data_link_degrade_rx_degrade_map_get(core_link, &rx_degrade_map);
	if (rtn == -EBADRQC)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "rx degrade lane map show (rx_degrade_map = 0x%X)", rx_degrade_map);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n", rx_degrade_map);
}

static ssize_t rx_link_speed_gbps_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	u16                  link_speed;
	bool                 is_rx_degraded;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_is_rx_degrade_get(core_link, &is_rx_degraded);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	if (!is_rx_degraded)
		return scnprintf(buf, PAGE_SIZE, "no\n");

	rtn = sl_core_data_link_degrade_rx_link_speed_get(core_link, &link_speed);
	if (rtn == -EBADRQC)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "rx degrade link speed gbps show (link_speed = %u)", link_speed);

	return scnprintf(buf, PAGE_SIZE, "%u\n", link_speed);
}

static ssize_t is_tx_degraded_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	bool                 is_tx_degraded;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_is_tx_degrade_get(core_link, &is_tx_degraded);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "is tx degraded show (degrade = %s)",
		   is_tx_degraded ? "yes" : "no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_tx_degraded ? "yes" : "no");
}

static ssize_t tx_degrade_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	u8                   tx_degrade_map;
	bool                 is_tx_degraded;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_is_tx_degrade_get(core_link, &is_tx_degraded);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	if (!is_tx_degraded)
		return scnprintf(buf, PAGE_SIZE, "no\n");

	rtn = sl_core_data_link_degrade_tx_degrade_map_get(core_link, &tx_degrade_map);
	if (rtn == -EBADRQC)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "tx degrade lane map show (tx_degrade_map = 0x%X)", tx_degrade_map);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n", tx_degrade_map);
}

static ssize_t tx_link_speed_gbps_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	u16                  link_speed;
	bool                 is_tx_degraded;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_is_tx_degrade_get(core_link, &is_tx_degraded);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	if (!is_tx_degraded)
		return scnprintf(buf, PAGE_SIZE, "no\n");

	rtn = sl_core_data_link_degrade_tx_link_speed_get(core_link, &link_speed);
	if (rtn == -EBADRQC)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "tx degrade link speed gbps show (link_speed = %u)", link_speed);

	return scnprintf(buf, PAGE_SIZE, "%u\n", link_speed);
}

static ssize_t is_recoverable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_core_link *core_link;
	bool                 is_recoverable;

	core_link = container_of(kobj, struct sl_core_link, degrade_kobj);

	rtn = sl_core_data_link_degrade_is_recoverable_get(core_link, &is_recoverable);
	if (rtn == -EBADRQC)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "inactive");
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "is recoverable show (is_recoverable = %s)", is_recoverable ? "yes" : "no");

	return scnprintf(buf, PAGE_SIZE, "%s\n", is_recoverable ? "yes" : "no");
}

static struct kobj_attribute link_degrade_state              = __ATTR_RO(state);
static struct kobj_attribute link_is_rx_degraded             = __ATTR_RO(is_rx_degraded);
static struct kobj_attribute link_rx_degrade_lane_map        = __ATTR_RO(rx_degrade_map);
static struct kobj_attribute link_rx_degrade_link_speed_gbps = __ATTR_RO(rx_link_speed_gbps);
static struct kobj_attribute link_is_tx_degraded             = __ATTR_RO(is_tx_degraded);
static struct kobj_attribute link_tx_degrade_lane_map        = __ATTR_RO(tx_degrade_map);
static struct kobj_attribute link_tx_degrade_link_speed_gbps = __ATTR_RO(tx_link_speed_gbps);
static struct kobj_attribute link_is_recoverable             = __ATTR_RO(is_recoverable);

static struct attribute *link_degrade_attrs[] = {
	&link_degrade_state.attr,
	&link_is_rx_degraded.attr,
	&link_rx_degrade_lane_map.attr,
	&link_rx_degrade_link_speed_gbps.attr,
	&link_is_tx_degraded.attr,
	&link_tx_degrade_lane_map.attr,
	&link_tx_degrade_link_speed_gbps.attr,
	&link_is_recoverable.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_degrade);

static struct kobj_type link_degrade = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_degrade_groups,
};

int sl_sysfs_link_degrade_create(struct sl_core_link *core_link, struct kobject *parent_kobj)
{
	int rtn;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "link degrade create (num = %u)", core_link->num);

	rtn = kobject_init_and_add(&core_link->degrade_kobj, &link_degrade, parent_kobj, "auto_lane_degrade");
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME,
			   "link degrade create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&core_link->degrade_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_degrade_delete(struct sl_core_link *core_link)
{
	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "link degrade delete (num = %u)", core_link->num);

	kobject_put(&core_link->degrade_kobj);
}
