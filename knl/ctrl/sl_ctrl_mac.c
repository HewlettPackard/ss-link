// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/slab.h>

#include <linux/sl_mac.h>

#include "sl_asic.h"
#include "sl_sysfs.h"
#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_mac.h"
#include "sl_core_mac.h"
#include "sl_core_str.h"

#define SL_CTRL_MAC_DEL_WAIT_TIMEOUT_MS 2000

#define LOG_NAME SL_CTRL_MAC_LOG_NAME

static struct sl_ctrl_mac *ctrl_macs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctrl_macs_lock);

int sl_ctrl_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num, struct kobject *sysfs_parent)
{
	int                 rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (ctrl_mac) {
		sl_ctrl_log_err(ctrl_mac, LOG_NAME, "exists (mac = 0x%p)", ctrl_mac);
		return -EBADRQC;
	}

	ctrl_mac = kzalloc(sizeof(struct sl_ctrl_mac), GFP_KERNEL);
	if (!ctrl_mac)
		return -ENOMEM;

	ctrl_mac->magic     = SL_CTRL_MAC_MAGIC;
	ctrl_mac->ver       = SL_CTRL_MAC_VER;
	ctrl_mac->num       = mac_num;
	ctrl_mac->ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);

	kref_init(&ctrl_mac->ref_cnt);
	init_completion(&ctrl_mac->del_complete);

	spin_lock_init(&(ctrl_mac->data_lock));

	rtn = sl_core_mac_new(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
			"core_mac_new failed [%d]", rtn);
		goto out;
	}

	if (sysfs_parent) {
		ctrl_mac->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_mac_create(ctrl_mac);
		if (rtn) {
			sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
				"sysfs_mac_create failed [%d]", rtn);
			goto out;
		}
	}

	spin_lock(&ctrl_macs_lock);
	ctrl_macs[ldev_num][lgrp_num][mac_num] = ctrl_mac;
	spin_unlock(&ctrl_macs_lock);

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "new (mac = 0x%p)", ctrl_mac);

	return 0;

out:
	kfree(ctrl_mac);
	return -ENOMEM;
}

static void sl_ctrl_mac_release(struct kref *kref)
{
	struct sl_ctrl_mac *ctrl_mac;
	u8                  ldev_num;
	u8                  lgrp_num;
	u8                  mac_num;

	ctrl_mac = container_of(kref, struct sl_ctrl_mac, ref_cnt);
	ldev_num = ctrl_mac->ctrl_lgrp->ctrl_ldev->num;
	lgrp_num = ctrl_mac->ctrl_lgrp->num;
	mac_num  = ctrl_mac->num;

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "release (ctrl_mac = 0x%p)", ctrl_mac);

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_mac_delete(ctrl_mac);

	sl_core_mac_del(ldev_num, lgrp_num, mac_num);

	spin_lock(&ctrl_macs_lock);
	ctrl_macs[ldev_num][lgrp_num][mac_num] = NULL;
	spin_unlock(&ctrl_macs_lock);

	kfree(ctrl_mac);
}

static int sl_ctrl_mac_put(struct sl_ctrl_mac *ctrl_mac)
{
	return kref_put(&ctrl_mac->ref_cnt, sl_ctrl_mac_release);
}

int sl_ctrl_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_ctrl_mac *ctrl_mac;
	unsigned long       timeleft;

	sl_ctrl_log_dbg(NULL, LOG_NAME, "del (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
		ldev_num, lgrp_num, mac_num);

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_err_trace(NULL, LOG_NAME,
			"mac not found (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
			ldev_num, lgrp_num, mac_num);
		return -EBADRQC;
	}

	/* Release occurs on the last caller. Block until complete. */
	if (!sl_ctrl_mac_put(ctrl_mac)) {
		timeleft = wait_for_completion_timeout(&ctrl_mac->del_complete,
			msecs_to_jiffies(SL_CTRL_MAC_DEL_WAIT_TIMEOUT_MS));

		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "del completion_timeout (timeleft = %lums)", timeleft);

		if (timeleft == 0) {
			sl_ctrl_log_err(ctrl_mac, LOG_NAME,
				"del completion_timeout (ctrl_mac = 0x%p)", ctrl_mac);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static bool sl_ctrl_mac_kref_get_unless_zero(struct sl_ctrl_mac *ctrl_mac)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctrl_mac->ref_cnt) != 0);

	if (!incremented)
		sl_ctrl_log_warn(ctrl_mac, LOG_NAME,
			"kref_get_unless_zero ref unavailable (ctrl_mac = 0x%p)", ctrl_mac);

	return incremented;
}

struct sl_ctrl_mac *sl_ctrl_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_ctrl_mac *ctrl_mac;

	spin_lock(&ctrl_macs_lock);
	ctrl_mac = ctrl_macs[ldev_num][lgrp_num][mac_num];
	spin_unlock(&ctrl_macs_lock);

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "get (mac = 0x%p)", ctrl_mac);

	return ctrl_mac;
}

int sl_ctrl_mac_tx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_err(NULL, LOG_NAME, "tx start - NULL mac");
		return -EBADRQC;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_err(ctrl_mac, LOG_NAME, "tx start - kref unavailable (ctrl_mac = 0x%p)",
			ctrl_mac);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx start");

	rtn = sl_core_mac_tx_start(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
			"tx start - core_mac_tx_start failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:
	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx start - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	return rtn;
}

int sl_ctrl_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "tx stop - NULL mac");
		return 0;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx stop - kref unavailable (ctrl_mac = 0x%p)",
			ctrl_mac);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx stop");

	rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
				"tx stop - core_mac_tx_stop failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:
	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx stop - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	return rtn;
}

int sl_ctrl_mac_tx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;
	u32                core_mac_state;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "tx state get - NULL mac");
		*state = SL_MAC_STATE_OFF;
		return 0;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME,
			"tx state get - kref unavailable (ctrl_mac = 0x%p)", ctrl_mac);
		*state = SL_MAC_STATE_OFF;
		return 0;
	}

	rtn = sl_core_mac_tx_state_get(ldev_num, lgrp_num, mac_num, &core_mac_state);
	if (rtn) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME,
			"tx state get - core_mac_tx_state_get failed [%d]", rtn);
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
	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "tx state get - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	return 0;
}

int sl_ctrl_mac_rx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_err(NULL, LOG_NAME, "rx start - NULL mac");
		return -EBADRQC;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_err(ctrl_mac, LOG_NAME, "rx start - kref unavailable (ctrl_mac = 0x%p)",
			ctrl_mac);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx start");

	rtn = sl_core_mac_rx_start(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
				"rx start - core_mac_rx_start failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:

	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx start - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	return rtn;
}

int sl_ctrl_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "rx stop - NULL mac");
		return 0;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME,
			"rx stop - kref unavailable (ctrl_mac = 0x%p)", ctrl_mac);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx stop");

	rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, mac_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
			"rx stop - core_mac_rx_stop failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:

	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx stop - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	return 0;
}

int sl_ctrl_mac_rx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state)
{
	int                rtn;
	struct sl_ctrl_mac *ctrl_mac;
	u32                core_mac_state;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "rx state get - NULL mac");
		*state = SL_MAC_STATE_OFF;
		return 0;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME,
			"rx state get - kref unavailable (ctrl_mac = 0x%p)", ctrl_mac);
		*state = SL_MAC_STATE_OFF;
		return 0;
	}

	rtn = sl_core_mac_rx_state_get(ldev_num, lgrp_num, mac_num, &core_mac_state);
	if (rtn) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME,
			"rx state get - core_mac_rx_state_get failed [%d]", rtn);
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

	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "rx state get - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	return 0;
}

int sl_ctrl_mac_reset(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                tx_rtn;
	int                rx_rtn;
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_dbg(NULL, LOG_NAME, "reset - NULL mac");
		return 0;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME,
			"reset - kref unavailable (ctrl_mac = 0x%p)", ctrl_mac);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "reset");

	tx_rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, mac_num);
	if (tx_rtn) {
		/* Log error, but allow the function to continue to mac rx stop */
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
			"reset - core_mac_tx_stop failed [%d]", tx_rtn);
	}

	rx_rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, mac_num);
	if (rx_rtn) {
		/* Log error, but allow the function to continue execution */
		sl_ctrl_log_err_trace(ctrl_mac, LOG_NAME,
			"core_mac_rx_stop failed [%d]", rx_rtn);
	}

	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "reset - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	if (tx_rtn && rx_rtn)
		return -EIO;

	return 0;
}

int sl_ctrl_mac_info_map_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u64 *info_map)
{
	struct sl_ctrl_mac *ctrl_mac;

	ctrl_mac = sl_ctrl_mac_get(ldev_num, lgrp_num, mac_num);
	if (!ctrl_mac) {
		sl_ctrl_log_err(NULL, LOG_NAME,
			"info map get NULL mac (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
			ldev_num, lgrp_num, mac_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_mac_kref_get_unless_zero(ctrl_mac)) {
		sl_ctrl_log_err(ctrl_mac, LOG_NAME,
			"info map get kref_get_unless_zero failed (ctrl_mac = 0x%p)", ctrl_mac);
		return -EBADRQC;
	}

	*info_map = sl_core_mac_info_map_get(ldev_num, lgrp_num, mac_num);

	sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "info map get (info_map = 0x%llX)", *info_map);

	if (sl_ctrl_mac_put(ctrl_mac))
		sl_ctrl_log_dbg(ctrl_mac, LOG_NAME, "info map get - mac removed (ctrl_mac = 0x%p)", ctrl_mac);

	return 0;
}
