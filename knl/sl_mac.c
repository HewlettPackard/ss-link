// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/err.h>

#include <linux/sl_mac.h>

#include "sl_asic.h"
#include "sl_log.h"
#include "sl_lgrp.h"
#include "sl_mac.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_mac.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_MAC_LOG_NAME

static struct sl_mac macs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];

void sl_mac_init(void)
{
	u8 ldev_num;
	u8 lgrp_num;
	u8 link_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
			for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
				macs[ldev_num][lgrp_num][link_num].magic    = SL_MAC_MAGIC;
				macs[ldev_num][lgrp_num][link_num].ver      = SL_MAC_VER;
				macs[ldev_num][lgrp_num][link_num].num      = link_num;
				macs[ldev_num][lgrp_num][link_num].lgrp_num = lgrp_num;
				macs[ldev_num][lgrp_num][link_num].ldev_num = ldev_num;
			}
		}
	}
}

static int sl_mac_check(struct sl_mac *mac)
{
	if (!mac) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL mac");
		return -EINVAL;
	}
	if (IS_ERR(mac)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "mac pointer error");
		return -EINVAL;
	}
	if (mac->magic != SL_MAC_MAGIC) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad mac magic");
		return -EINVAL;
	}
	if (mac->ver != SL_MAC_VER) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad mac version");
		return -EINVAL;
	}
	if (mac->num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad mac num");
		return -EINVAL;
	}
	if (mac->lgrp_num >= SL_ASIC_MAX_LGRPS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad lgrp num");
		return -EINVAL;
	}
	if (mac->ldev_num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "bad ldev num");
		return -EINVAL;
	}

	return 0;
}

struct sl_mac *sl_mac_new(struct sl_lgrp *lgrp, u8 mac_num, struct kobject *sysfs_parent)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "new fail");
		return ERR_PTR(rtn);
	}
	if (mac_num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"invalid (mac_num = %u)", mac_num);
		return ERR_PTR(-EINVAL);
	}

	rtn = sl_ctl_mac_new(lgrp->ldev_num, lgrp->num, mac_num, sysfs_parent);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"ctl_mac_new failed [%d]", rtn);
		return ERR_PTR(-EINVAL);
	}

	return &macs[lgrp->ldev_num][lgrp->num][mac_num];
}
EXPORT_SYMBOL(sl_mac_new);

int sl_mac_del(struct sl_mac *mac)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "del fail");
		return rtn;
	}

	return sl_ctl_mac_del(mac->ldev_num, mac->lgrp_num, mac->num);
}
EXPORT_SYMBOL(sl_mac_del);

int sl_mac_tx_start(struct sl_mac *mac)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "tx start fail");
		return rtn;
	}

	return sl_ctl_mac_tx_start(mac->ldev_num, mac->lgrp_num, mac->num);
}
EXPORT_SYMBOL(sl_mac_tx_start);

int sl_mac_tx_stop(struct sl_mac *mac)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "tx stop fail");
		return rtn;
	}

	return sl_ctl_mac_tx_stop(mac->ldev_num, mac->lgrp_num, mac->num);
}
EXPORT_SYMBOL(sl_mac_tx_stop);

int sl_mac_tx_state_get(struct sl_mac *mac, u32 *state)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "tx state get fail");
		return rtn;
	}

	return sl_ctl_mac_tx_state_get(mac->ldev_num, mac->lgrp_num, mac->num, state);
}
EXPORT_SYMBOL(sl_mac_tx_state_get);

int sl_mac_rx_start(struct sl_mac *mac)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "rx start fail");
		return rtn;
	}

	return sl_ctl_mac_rx_start(mac->ldev_num, mac->lgrp_num, mac->num);
}
EXPORT_SYMBOL(sl_mac_rx_start);

int sl_mac_rx_stop(struct sl_mac *mac)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "rx stop fail");
		return rtn;
	}

	return sl_ctl_mac_rx_stop(mac->ldev_num, mac->lgrp_num, mac->num);
}
EXPORT_SYMBOL(sl_mac_rx_stop);

int sl_mac_rx_state_get(struct sl_mac *mac, u32 *state)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "rx state get fail");
		return rtn;
	}

	return sl_ctl_mac_rx_state_get(mac->ldev_num, mac->lgrp_num, mac->num, state);
}
EXPORT_SYMBOL(sl_mac_rx_state_get);

int sl_mac_reset(struct sl_mac *mac)
{
	int rtn;

	rtn = sl_mac_check(mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "reset fail");
		return rtn;
	}

	return sl_ctl_mac_reset(mac->ldev_num, mac->lgrp_num, mac->num);
}
EXPORT_SYMBOL(sl_mac_reset);

const char *sl_mac_state_str(u32 state)
{
	switch (state) {
	case SL_MAC_STATE_OFF:
		return "off";
	case SL_MAC_STATE_ON:
		return "on";
	default:
		return "unknown";
	}
}
EXPORT_SYMBOL(sl_mac_state_str);
