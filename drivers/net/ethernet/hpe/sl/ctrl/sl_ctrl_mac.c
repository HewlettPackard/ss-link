// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/bitmap.h>
#include <linux/types.h>
#include <linux/slab.h>

#include <linux/hpe/sl/sl_mac.h>

#include "sl_asic.h"
#include "sl_sysfs.h"
#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_mac.h"
#include "sl_core_mac.h"
#include "sl_core_str.h"
#include "sl_ctrl_mac_counters.h"

#define SL_CTRL_MAC_DEL_WAIT_TIMEOUT_MS 2000

#define LOG_NAME SL_CTRL_MAC_LOG_NAME

static struct sl_ctrl_mac *ctrl_macs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctrl_macs_lock);
static DECLARE_BITMAP(ctrl_macs_busy, SL_ASIC_MAX_LDEVS * SL_ASIC_MAX_LGRPS * SL_ASIC_MAX_LINKS);

static inline unsigned int sl_ctrl_mac_busy_bit(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	return (ldev_num * SL_ASIC_MAX_LGRPS + lgrp_num) * SL_ASIC_MAX_LINKS + mac_num;
}

static struct sl_ctrl_mac *sl_ctrl_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_ctrl_mac *ctrl_mac;

	spin_lock(&ctrl_macs_lock);
	ctrl_mac = ctrl_macs[ldev_num][lgrp_num][mac_num];
	spin_unlock(&ctrl_macs_lock);

	return ctrl_mac;
}

static void sl_ctrl_mac_release(struct kref *kref)
{
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = container_of(kref, struct sl_ctrl_mac, ref_cnt);

	complete(&ctrl_mac->no_refs);
}

int sl_ctrl_mac_put_ref(struct sl_ctrl_mac *ctrl_mac)
{
	return kref_put(&ctrl_mac->ref_cnt, sl_ctrl_mac_release);
}

int sl_ctrl_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num, struct kobject *sysfs_parent)
{
	int                  rtn;
	struct sl_ctrl_mac  *ctrl_mac;
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ctrl_lgrp = sl_ctrl_lgrp_get_ref(ldev_num, lgrp_num);
	if (!ctrl_lgrp)
		return -EBADRQC;

	if (test_and_set_bit_lock(sl_ctrl_mac_busy_bit(ldev_num, lgrp_num, mac_num), ctrl_macs_busy)) {
		sl_ctrl_log_err(NULL, LOG_NAME,
				"mac_new busy (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
				ldev_num, lgrp_num, mac_num);
		sl_ctrl_lgrp_put_ref(ctrl_lgrp);
		return -EBADRQC;
	}

	if (sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num)) {
		sl_ctrl_log_err(NULL, LOG_NAME,
				"mac_new exists (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
				ldev_num, lgrp_num, mac_num);
		rtn = -EBADRQC;
		goto out_unlock;
	}

	ctrl_mac = kzalloc(sizeof(*ctrl_mac), GFP_KERNEL);
	if (!ctrl_mac) {
		rtn = -ENOMEM;
		goto out_unlock;
	}

	ctrl_mac->magic     = SL_CTRL_MAC_MAGIC;
	ctrl_mac->ver       = SL_CTRL_MAC_VER;
	ctrl_mac->num       = mac_num;
	ctrl_mac->ctrl_lgrp = ctrl_lgrp;

	kref_init(&ctrl_mac->ref_cnt);
	init_completion(&ctrl_mac->no_refs);

	spin_lock_init(&ctrl_mac->data_lock);

	rtn = sl_ctrl_mac_counters_init(ctrl_mac);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "ctrl_mac_counters_init failed [%d]", rtn);
		goto out_free;
	}

	rtn = sl_core_mac_new(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "core_mac_new failed [%d]", rtn);
		goto out_mac_counters_del;
	}

	if (sysfs_parent) {
		ctrl_mac->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_mac_create(ctrl_mac);
		if (rtn) {
			sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "sysfs_mac_create failed [%d]", rtn);
			goto out_mac_del;
		}
	}

	spin_lock(&ctrl_macs_lock);
	ctrl_macs[ldev_num][lgrp_num][mac_num] = ctrl_mac;
	spin_unlock(&ctrl_macs_lock);

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "new (mac = 0x%p)", ctrl_mac);
	clear_bit_unlock(sl_ctrl_mac_busy_bit(ldev_num, lgrp_num, mac_num), ctrl_macs_busy);
	sl_ctrl_lgrp_put_ref(ctrl_lgrp);

	return 0;

out_mac_del:
	sl_core_mac_del(ldev_num, lgrp_num, mac_num);
out_mac_counters_del:
	sl_ctrl_mac_counters_del(ctrl_mac);
out_free:
	kfree(ctrl_mac);
out_unlock:
	clear_bit_unlock(sl_ctrl_mac_busy_bit(ldev_num, lgrp_num, mac_num), ctrl_macs_busy);
	sl_ctrl_lgrp_put_ref(ctrl_lgrp);
	return rtn;
}

static void sl_ctrl_mac_remove(struct sl_ctrl_mac *ctrl_mac)
{
	u8 ldev_num;
	u8 lgrp_num;
	u8 mac_num;

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "remove (ctrl_mac = 0x%p)", ctrl_mac);

	ldev_num = ctrl_mac->ctrl_lgrp->ctrl_ldev->num;
	lgrp_num = ctrl_mac->ctrl_lgrp->num;
	mac_num  = ctrl_mac->num;

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_mac_delete(ctrl_mac);

	sl_core_mac_del(ldev_num, lgrp_num, mac_num);

	sl_ctrl_mac_counters_del(ctrl_mac);

	spin_lock(&ctrl_macs_lock);
	ctrl_macs[ldev_num][lgrp_num][mac_num] = NULL;
	spin_unlock(&ctrl_macs_lock);

	kfree(ctrl_mac);
}

int sl_ctrl_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_ctrl_mac *ctrl_mac;
	int                 rtn;
	unsigned long       timeleft_jiffies;

	sl_ctrl_log_dbg(NULL, LOG_NAME, "del (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
			ldev_num, lgrp_num, mac_num);

	if (test_and_set_bit_lock(sl_ctrl_mac_busy_bit(ldev_num, lgrp_num, mac_num), ctrl_macs_busy)) {
		sl_ctrl_log_err(NULL, LOG_NAME,
				"del busy (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
				ldev_num, lgrp_num, mac_num);
		return -EBADRQC;
	}

	spin_lock(&ctrl_macs_lock);
	ctrl_mac = ctrl_macs[ldev_num][lgrp_num][mac_num];
	spin_unlock(&ctrl_macs_lock);
	if (!ctrl_mac) {
		sl_ctrl_log_err_trace(NULL, LOG_NAME, "mac not found (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
				      ldev_num, lgrp_num, mac_num);
		rtn = -EBADRQC;
		goto out;
	}

	kref_put(&ctrl_mac->ref_cnt, sl_ctrl_mac_release);

	timeleft_jiffies = wait_for_completion_timeout(&ctrl_mac->no_refs,
						       msecs_to_jiffies(SL_CTRL_MAC_DEL_WAIT_TIMEOUT_MS));
	if (!timeleft_jiffies)
		sl_ctrl_log_err(ctrl_mac, LOG_NAME,
				"del timed out, forcing remove (ref_cnt = %u)",
				kref_read(&ctrl_mac->ref_cnt));

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME,
			"waited for refs for %d ms (timeleft_jiffies = %lu)",
			SL_CTRL_MAC_DEL_WAIT_TIMEOUT_MS - jiffies_to_msecs(timeleft_jiffies), timeleft_jiffies);

	sl_ctrl_mac_remove(ctrl_mac);

	rtn = 0;
out:
	clear_bit_unlock(sl_ctrl_mac_busy_bit(ldev_num, lgrp_num, mac_num), ctrl_macs_busy);

	sl_ctrl_log_dbg(NULL, LOG_NAME, "del complete (ctrl_mac = 0x%p)", ctrl_mac);

	return rtn;
}

struct sl_ctrl_mac *sl_ctrl_mac_get_ref(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_ctrl_mac *ctrl_mac;

	spin_lock(&ctrl_macs_lock);
	ctrl_mac = ctrl_macs[ldev_num][lgrp_num][mac_num];

	/* If we are busy we are deleting/creating the object */
	if (ctrl_mac && (test_bit(sl_ctrl_mac_busy_bit(ldev_num, lgrp_num, mac_num), ctrl_macs_busy) ||
	    !kref_get_unless_zero(&ctrl_mac->ref_cnt)))
		ctrl_mac = NULL;

	spin_unlock(&ctrl_macs_lock);
	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "get ref (mac = 0x%p)", ctrl_mac);

	return ctrl_mac;
}

int sl_ctrl_mac_tx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_err(NULL, LOG_NAME, "tx start - NULL mac");
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx start");

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_TX_START_CMD);

	rtn = sl_core_mac_tx_start(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "tx start - core_mac_tx_start failed [%d]", rtn);
		SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_TX_START_FAIL);
		goto out;
	}

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_TX_STARTED);

	rtn = 0;

out:
	sl_ctrl_mac_put_ref(ctrl_mac);

	return rtn;
}

int sl_ctrl_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "tx stop - NULL mac");
		return 0;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx stop");

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_TX_STOP_CMD);

	rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "tx stop - core_mac_tx_stop failed [%d]", rtn);
		SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_TX_STOP_FAIL);
		goto out;
	}

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_TX_STOPPED);

	rtn = 0;

out:
	sl_ctrl_mac_put_ref(ctrl_mac);

	return rtn;
}

int sl_ctrl_mac_tx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;
	u32                core_mac_state;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "tx state get - NULL mac");
		*state = SL_MAC_STATE_OFF;
		return 0;
	}

	rtn = sl_core_mac_tx_state_get(ldev_num, lgrp_num, mac_num, &core_mac_state);
	if (rtn) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx state get - core_mac_tx_state_get failed [%d]", rtn);
		*state = SL_MAC_STATE_OFF;
		goto out;
	}

	switch (core_mac_state) {
	case SL_CORE_MAC_STATE_ON:
		*state = SL_MAC_STATE_ON;
		break;
	default:
		*state = SL_MAC_STATE_OFF;
		break;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx state get (state = %u %s, core_state = %u)",
			*state, sl_mac_state_str(*state), core_mac_state);

out:
	sl_ctrl_mac_put_ref(ctrl_mac);

	return 0;
}

int sl_ctrl_mac_rx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_err(NULL, LOG_NAME, "rx start - NULL mac");
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx start");

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RX_START_CMD);

	rtn = sl_core_mac_rx_start(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "rx start - core_mac_rx_start failed [%d]", rtn);
		SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RX_START_FAIL);
		goto out;
	}

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RX_STARTED);

	rtn = 0;

out:
	sl_ctrl_mac_put_ref(ctrl_mac);

	return rtn;
}

int sl_ctrl_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "rx stop - NULL mac");
		return 0;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx stop");

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RX_STOP_CMD);

	rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "rx stop - core_mac_rx_stop failed [%d]", rtn);
		SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RX_STOP_FAIL);
		goto out;
	}

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RX_STOPPED);

	rtn = 0;

out:
	sl_ctrl_mac_put_ref(ctrl_mac);

	return rtn;
}

int sl_ctrl_mac_rx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;
	u32                core_mac_state;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "rx state get - NULL mac");
		*state = SL_MAC_STATE_OFF;
		return 0;
	}

	rtn = sl_core_mac_rx_state_get(ldev_num, lgrp_num, mac_num, &core_mac_state);
	if (rtn) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx state get - core_mac_rx_state_get failed [%d]", rtn);
		*state = SL_MAC_STATE_OFF;
		goto out;
	}

	switch (core_mac_state) {
	case SL_CORE_MAC_STATE_ON:
		*state = SL_MAC_STATE_ON;
		break;
	default:
		*state = SL_MAC_STATE_OFF;
		break;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx state get (state = %u %s, core_state = %u)",
			*state, sl_mac_state_str(*state), core_mac_state);

out:
	sl_ctrl_mac_put_ref(ctrl_mac);

	return 0;
}

int sl_ctrl_mac_reset(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                tx_rtn;
	int                rx_rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "reset - NULL mac");
		return 0;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "reset");

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RESET_CMD);

	tx_rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, mac_num);
	if (tx_rtn) {
		/* Log error, but allow the function to continue to mac rx stop */
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "reset - core_mac_tx_stop failed [%d]", tx_rtn);
	}

	rx_rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, mac_num);
	if (rx_rtn) {
		/* Log error, but allow the function to continue execution */
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME, "core_mac_rx_stop failed [%d]", rx_rtn);
	}

	if (tx_rtn || rx_rtn) {
		SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RESET_FAIL);
		sl_ctrl_mac_put_ref(ctrl_mac);
		return -EIO;
	}

	SL_CTRL_MAC_COUNTER_INC(ctrl_mac, MAC_RESET);

	sl_ctrl_mac_put_ref(ctrl_mac);

	return 0;
}

int sl_ctrl_mac_info_map_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u64 *info_map)
{
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get_ref(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_err(NULL, LOG_NAME, "info map get NULL mac (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
				ldev_num, lgrp_num, mac_num);
		return -EBADRQC;
	}

	*info_map = sl_core_mac_info_map_get(ldev_num, lgrp_num, mac_num);

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "info map get (info_map = 0x%llX)", *info_map);

	sl_ctrl_mac_put_ref(ctrl_mac);

	return 0;
}
