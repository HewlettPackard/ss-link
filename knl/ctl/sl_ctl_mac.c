// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/sl_mac.h>

#include "sl_asic.h"
#include "sl_sysfs.h"

#include "base/sl_ctl_log.h"

#include "sl_ctl_lgrp.h"
#include "sl_ctl_mac.h"
#include "sl_core_mac.h"
#include "sl_core_str.h"

#define LOG_NAME SL_CTL_MAC_LOG_NAME

static struct sl_ctl_mac *ctl_macs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctl_macs_lock);

static void sl_ctl_mac_is_deleting_set(struct sl_ctl_mac *ctl_mac)
{
	unsigned long flags;

	spin_lock_irqsave(&ctl_mac->data_lock, flags);
	ctl_mac->is_deleting = true;
	spin_unlock_irqrestore(&ctl_mac->data_lock, flags);
}

static bool sl_ctl_mac_is_deleting(struct sl_ctl_mac *ctl_mac)
{
	unsigned long flags;
	bool          is_deleting;

	spin_lock_irqsave(&ctl_mac->data_lock, flags);
	is_deleting = ctl_mac->is_deleting;
	spin_unlock_irqrestore(&ctl_mac->data_lock, flags);

	return is_deleting;
}

int sl_ctl_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num, struct kobject *sysfs_parent)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (ctl_mac) {
		sl_ctl_log_err(ctl_mac, LOG_NAME, "exists (mac = 0x%p, is_deleting = %s)",
			ctl_mac, sl_ctl_mac_is_deleting(ctl_mac) ? "true" : "false");
		return -EBADRQC;
	}

	ctl_mac = kzalloc(sizeof(struct sl_ctl_mac), GFP_KERNEL);
	if (!ctl_mac)
		return -ENOMEM;

	ctl_mac->magic    = SL_CTL_MAC_MAGIC;
	ctl_mac->ver      = SL_CTL_MAC_VER;
	ctl_mac->num      = mac_num;
	ctl_mac->ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);

	spin_lock_init(&(ctl_mac->data_lock));

	rtn = sl_core_mac_new(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctl_log_err(ctl_mac, LOG_NAME,
			"core_mac_new failed [%d]", rtn);
		goto out;
	}

	if (sysfs_parent) {
		ctl_mac->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_mac_create(ctl_mac);
		if (rtn) {
			sl_ctl_log_err(ctl_mac, LOG_NAME,
				"sysfs_mac_create failed [%d]", rtn);
			goto out;
		}
	}

	spin_lock(&ctl_macs_lock);
	ctl_macs[ldev_num][lgrp_num][mac_num] = ctl_mac;
	spin_unlock(&ctl_macs_lock);

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "new (mac = 0x%p)", ctl_mac);

	return 0;

out:
	kfree(ctl_mac);
	return -ENOMEM;
}

void sl_ctl_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_dbg(NULL, LOG_NAME, "not found (mac_num = %u)", mac_num);
		return;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "del (mac = 0x%p)", ctl_mac);

	if (sl_ctl_mac_is_deleting(ctl_mac)) {
		sl_ctl_log_dbg(ctl_mac, LOG_NAME, "del in progress");
		return;
	}

	sl_ctl_mac_is_deleting_set(ctl_mac);

	rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, mac_num);
	if (rtn)
		sl_ctl_log_warn(ctl_mac, LOG_NAME, "core_mac_rx_stop failed [%d]", rtn);
	rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, mac_num);
	if (rtn)
		sl_ctl_log_warn(ctl_mac, LOG_NAME, "core_mac_tx_stop failed [%d]", rtn);

	sl_core_mac_del(ldev_num, lgrp_num, mac_num);
	if (rtn)
		sl_ctl_log_warn(ctl_mac, LOG_NAME, "core_mac_del failed [%d]", rtn);

	sl_sysfs_mac_delete(ctl_mac);

	spin_lock(&ctl_macs_lock);
	ctl_macs[ldev_num][lgrp_num][mac_num] = NULL;
	spin_unlock(&ctl_macs_lock);

	kfree(ctl_mac);
}

struct sl_ctl_mac *sl_ctl_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	unsigned long      irq_flags;
	struct sl_ctl_mac *ctl_mac;

	spin_lock_irqsave(&ctl_macs_lock, irq_flags);
	ctl_mac = ctl_macs[ldev_num][lgrp_num][mac_num];
	spin_unlock_irqrestore(&ctl_macs_lock, irq_flags);

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "get (mac = 0x%p)", ctl_mac);

	return ctl_mac;
}

int sl_ctl_mac_tx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_err(NULL, LOG_NAME, "tx start - NULL mac");
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "tx start");

	rtn = sl_core_mac_tx_start(ldev_num, lgrp_num, mac_num);
	switch (rtn) {
	case 0:
	case -ENOLINK:
		break;
	default:
		sl_ctl_log_err(ctl_mac, LOG_NAME,
			"tx start - core_mac_tx_start failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_ctl_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_dbg(NULL, LOG_NAME, "tx stop - NULL mac");
		return 0;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "tx stop");

	rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, mac_num);
	switch (rtn) {
	case 0:
	case -ENOLINK:
		break;
	default:
		sl_ctl_log_err(ctl_mac, LOG_NAME,
			"tx stop - core_mac_tx_stop failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_ctl_mac_tx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;
	u32                core_mac_state;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_dbg(NULL, LOG_NAME, "tx state get - NULL mac");
		return SL_MAC_STATE_OFF;
	}

	rtn = sl_core_mac_tx_state_get(ldev_num, lgrp_num, mac_num, &core_mac_state);
	if (rtn) {
		sl_ctl_log_dbg(ctl_mac, LOG_NAME,
			"tx state get - core_mac_tx_state_get failed [%d]", rtn);
		return SL_MAC_STATE_OFF;
	}

	switch (core_mac_state) {
	case SL_CORE_MAC_STATE_ON:
		*state = SL_MAC_STATE_ON;
		break;
	default:
		*state = SL_MAC_STATE_OFF;
		break;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "tx state get (state = %u %s)",
		*state, sl_mac_state_str(*state));

	return 0;
}

int sl_ctl_mac_rx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_err(NULL, LOG_NAME, "rx start - NULL mac");
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "rx start");

	rtn = sl_core_mac_rx_start(ldev_num, lgrp_num, mac_num);
	switch (rtn) {
	case 0:
	case -ENOLINK:
		break;
	default:
		sl_ctl_log_err(ctl_mac, LOG_NAME,
			"rx start - core_mac_rx_start failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_ctl_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_dbg(NULL, LOG_NAME, "rx stop - NULL mac");
		return 0;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "rx stop");

	rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, mac_num);
	switch (rtn) {
	case 0:
	case -ENOLINK:
		break;
	default:
		sl_ctl_log_err(ctl_mac, LOG_NAME,
			"rx stop - core_mac_rx_stop failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_ctl_mac_rx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state)
{
	int                rtn;
	struct sl_ctl_mac *ctl_mac;
	u32                core_mac_state;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_dbg(NULL, LOG_NAME, "rx state get - NULL mac");
		return SL_MAC_STATE_OFF;
	}

	rtn = sl_core_mac_rx_state_get(ldev_num, lgrp_num, mac_num, &core_mac_state);
	if (rtn) {
		sl_ctl_log_dbg(ctl_mac, LOG_NAME,
			"rx state get - core_mac_rx_state_get failed [%d]", rtn);
		return SL_MAC_STATE_OFF;
	}

	switch (core_mac_state) {
	case SL_CORE_MAC_STATE_ON:
		*state = SL_MAC_STATE_ON;
		break;
	default:
		*state = SL_MAC_STATE_OFF;
		break;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "rx state get (state = %u %s)",
		*state, sl_mac_state_str(*state));

	return 0;
}

int sl_ctl_mac_reset(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                tx_rtn;
	int                rx_rtn;
	struct sl_ctl_mac *ctl_mac;

	ctl_mac = sl_ctl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctl_mac) {
		sl_ctl_log_dbg(NULL, LOG_NAME, "reset - NULL mac");
		return 0;
	}

	sl_ctl_log_dbg(ctl_mac, LOG_NAME, "reset");

	tx_rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, mac_num);
	switch (tx_rtn) {
	case 0:
	case -ENOLINK:
		break;
	default:
		/* Log error, but allow the function to continue to mac rx stop */
		sl_ctl_log_err(ctl_mac, LOG_NAME,
			"reset - core_mac_tx_stop failed [%d]", tx_rtn);
		break;
	}

	rx_rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, mac_num);
	switch (rx_rtn) {
	case 0:
	case -ENOLINK:
		break;
	default:
		/* Log error, but allow the function to continue execution */
		sl_ctl_log_err(ctl_mac, LOG_NAME,
			"core_mac_rx_stop failed [%d]", rx_rtn);
		break;
	}

	if (((tx_rtn != 0) && (tx_rtn != -ENOLINK)) ||
		((rx_rtn != 0) && (rx_rtn != -ENOLINK)))
		return -EIO;

	return 0;
}
