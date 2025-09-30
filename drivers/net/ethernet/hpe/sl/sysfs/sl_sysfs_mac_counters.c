// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_sysfs.h"
#include "sl_log.h"
#include "sl_ctrl_mac.h"
#include "sl_ctrl_mac_counters.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t mac_tx_start_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_TX_START_CMD);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac tx start cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_tx_started_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_TX_STARTED);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac tx started show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_tx_start_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_TX_START_FAIL);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac tx start fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_tx_stop_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_TX_STOP_CMD);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac tx stop cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_tx_stopped_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_TX_STOPPED);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac tx stopped show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_tx_stop_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_TX_STOP_FAIL);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac tx stop fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_rx_start_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RX_START_CMD);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac rx start cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_rx_started_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RX_STARTED);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac rx started show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_rx_start_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RX_START_FAIL);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac rx start fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_rx_stop_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RX_STOP_CMD);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac rx stop cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_rx_stopped_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RX_STOPPED);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac rx stopped show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_rx_stop_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RX_STOP_FAIL);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac rx stop fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_reset_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RESET_CMD);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac reset cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_reset_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RESET);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac reset show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t mac_reset_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_mac *ctrl_mac;
	u32                 counter;

	ctrl_mac = container_of(kobj, struct sl_ctrl_mac, counters_kobj);

	counter = sl_ctrl_mac_counter_get(ctrl_mac, MAC_RESET_FAIL);

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac reset fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static struct kobj_attribute mac_tx_start_cmd  = __ATTR_RO(mac_tx_start_cmd);
static struct kobj_attribute mac_tx_started    = __ATTR_RO(mac_tx_started);
static struct kobj_attribute mac_tx_start_fail = __ATTR_RO(mac_tx_start_fail);
static struct kobj_attribute mac_tx_stop_cmd   = __ATTR_RO(mac_tx_stop_cmd);
static struct kobj_attribute mac_tx_stopped    = __ATTR_RO(mac_tx_stopped);
static struct kobj_attribute mac_tx_stop_fail  = __ATTR_RO(mac_tx_stop_fail);
static struct kobj_attribute mac_rx_start_cmd  = __ATTR_RO(mac_rx_start_cmd);
static struct kobj_attribute mac_rx_started    = __ATTR_RO(mac_rx_started);
static struct kobj_attribute mac_rx_start_fail = __ATTR_RO(mac_rx_start_fail);
static struct kobj_attribute mac_rx_stop_cmd   = __ATTR_RO(mac_rx_stop_cmd);
static struct kobj_attribute mac_rx_stopped    = __ATTR_RO(mac_rx_stopped);
static struct kobj_attribute mac_rx_stop_fail  = __ATTR_RO(mac_rx_stop_fail);
static struct kobj_attribute mac_reset_cmd     = __ATTR_RO(mac_reset_cmd);
static struct kobj_attribute mac_reset         = __ATTR_RO(mac_reset);
static struct kobj_attribute mac_reset_fail    = __ATTR_RO(mac_reset_fail);

static struct attribute *mac_counters_attrs[] = {
	&mac_tx_start_cmd.attr,
	&mac_tx_started.attr,
	&mac_tx_start_fail.attr,
	&mac_tx_stop_cmd.attr,
	&mac_tx_stopped.attr,
	&mac_tx_stop_fail.attr,
	&mac_rx_start_cmd.attr,
	&mac_rx_started.attr,
	&mac_rx_start_fail.attr,
	&mac_rx_stop_cmd.attr,
	&mac_rx_stopped.attr,
	&mac_rx_stop_fail.attr,
	&mac_reset_cmd.attr,
	&mac_reset.attr,
	&mac_reset_fail.attr,
	NULL
};
ATTRIBUTE_GROUPS(mac_counters);

static struct kobj_type mac_counters = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = mac_counters_groups,
};

int sl_sysfs_mac_counters_create(struct sl_ctrl_mac *ctrl_mac)
{
	int rtn;

	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac counters create");

	rtn = kobject_init_and_add(&ctrl_mac->counters_kobj, &mac_counters, &ctrl_mac->kobj, "counters");
	if (rtn) {
		sl_log_err(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac counters create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_mac->counters_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_mac_counters_delete(struct sl_ctrl_mac *ctrl_mac)
{
	sl_log_dbg(ctrl_mac, LOG_BLOCK, LOG_NAME, "mac counters delete");
	kobject_put(&ctrl_mac->counters_kobj);
}
