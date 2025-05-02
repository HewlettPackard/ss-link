// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "sl_kconfig.h"

#include "uapi/sl_media.h"
#include "uapi/sl_lgrp.h"

#include "base/sl_core_log.h"

#include "sl_core_mac.h"
#include "sl_core_str.h"
#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_link.h"
#include "data/sl_core_data_mac.h"
#include "hw/sl_core_hw_mac.h"
#include "hw/sl_core_hw_settings.h"

static struct sl_core_mac *core_macs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(core_macs_lock);

#define LOG_NAME SL_CORE_DATA_MAC_LOG_NAME

int sl_core_data_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_core_mac *core_mac;

	core_mac = sl_core_data_mac_get(ldev_num, lgrp_num, mac_num);
	if (core_mac) {
		sl_core_log_err(core_mac, LOG_NAME, "exists (mac = 0x%p)", core_mac);
		return -EBADRQC;
	}

	core_mac = kzalloc(sizeof(struct sl_core_mac), GFP_KERNEL);
	if (!core_mac)
		return -ENOMEM;

	core_mac->magic     = SL_CORE_MAC_MAGIC;
	core_mac->num       = mac_num;
	core_mac->core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	spin_lock_init(&core_mac->data_lock);

	sl_core_hw_mac_tx_stop(core_mac);
	sl_core_hw_mac_rx_stop(core_mac);

	sl_core_log_dbg(core_mac, LOG_NAME, "new (link = 0x%p)", core_mac);

	spin_lock(&core_macs_lock);
	core_macs[ldev_num][lgrp_num][mac_num] = core_mac;
	spin_unlock(&core_macs_lock);

	return 0;
}

void sl_core_data_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);
	if (!core_mac) {
		sl_core_log_dbg(NULL, LOG_NAME, "not found (mac_num = %u)", mac_num);
		return;
	}

	sl_core_hw_mac_tx_stop(core_mac);
	sl_core_hw_mac_rx_stop(core_mac);

	spin_lock(&core_macs_lock);
	core_macs[ldev_num][lgrp_num][mac_num] = NULL;
	spin_unlock(&core_macs_lock);

	sl_core_log_dbg(core_mac, LOG_NAME, "del (mac = 0x%p)", core_mac);

	kfree(core_mac);
}

struct sl_core_mac *sl_core_data_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	unsigned long       irq_flags;
	struct sl_core_mac *core_mac;

	spin_lock_irqsave(&core_macs_lock, irq_flags);
	core_mac = core_macs[ldev_num][lgrp_num][mac_num];
	spin_unlock_irqrestore(&core_macs_lock, irq_flags);

	sl_core_log_dbg(core_mac, LOG_NAME, "get (mac = 0x%p)", core_mac);

	return core_mac;
}

int sl_core_data_mac_tx_settings(struct sl_core_mac *core_mac)
{
	struct sl_lgrp_config *lgrp_config;
	struct sl_link_caps   *link_caps;

	sl_core_log_dbg(core_mac, LOG_NAME, "tx settings");

	lgrp_config = &(core_mac->core_lgrp->config);
	link_caps   = &(core_mac->core_lgrp->link_caps[core_mac->num]);

	if (hweight_long(link_caps->tech_map) != 1) {
		sl_core_log_err(core_mac, LOG_NAME,
			"tx settings - tech map invalid (map = 0x%08X)",
			link_caps->tech_map);
		return -EINVAL;
	}

	if (SL_LGRP_CONFIG_TECH_CK_400G & link_caps->tech_map)
		core_mac->settings.tx_ifg_adj = SL_CORE_HW_IFG_ADJ_400G;
	if (SL_LGRP_CONFIG_TECH_CK_200G & link_caps->tech_map)
		core_mac->settings.tx_ifg_adj = SL_CORE_HW_IFG_ADJ_200G;
	if (SL_LGRP_CONFIG_TECH_CK_100G & link_caps->tech_map)
		core_mac->settings.tx_ifg_adj = SL_CORE_HW_IFG_ADJ_100G;
	if (SL_LGRP_CONFIG_TECH_BS_200G & link_caps->tech_map)
		core_mac->settings.tx_ifg_adj = SL_CORE_HW_IFG_ADJ_200G;
	if (SL_LGRP_CONFIG_TECH_CD_100G & link_caps->tech_map)
		core_mac->settings.tx_ifg_adj = SL_CORE_HW_IFG_ADJ_100G;
	if (SL_LGRP_CONFIG_TECH_CD_50G  & link_caps->tech_map)
		core_mac->settings.tx_ifg_adj = SL_CORE_HW_IFG_ADJ_50G;
	if (SL_LGRP_CONFIG_TECH_BJ_100G & link_caps->tech_map)
		core_mac->settings.tx_ifg_adj = SL_CORE_HW_IFG_ADJ_100G;

	sl_core_log_dbg(core_mac, LOG_NAME, "tx settings (ifg = %u)", core_mac->settings.tx_ifg_adj);

	if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_FABRIC)) {
		core_mac->settings.tx_ifg_adj          = SL_CORE_HW_IFG_ADJ_NONE;
		core_mac->settings.tx_ifg_mode         = 0;
		core_mac->settings.tx_pad_idle_thresh  = 10;
		core_mac->settings.tx_idle_delay       = 0;
		core_mac->settings.tx_priority_thresh  = 0;
		core_mac->settings.tx_cdt_thresh_2     = 0;
		core_mac->settings.tx_cdt_thresh       = 15;
		core_mac->settings.tx_cdt_init_val     = 72;
		core_mac->settings.tx_pcs_credits      = 8;
		core_mac->settings.tx_short_preamble   = 1;
		core_mac->settings.llr_if_credits      = 8;
	} else {
		core_mac->settings.tx_ifg_mode         = 1;
		core_mac->settings.tx_pad_idle_thresh  = 10;
		switch (lgrp_config->furcation) {
		case SL_MEDIA_FURCATION_X1:
			core_mac->settings.tx_idle_delay      = 0;
			core_mac->settings.tx_priority_thresh = 0;
			core_mac->settings.tx_cdt_thresh_2    = 0;
			core_mac->settings.tx_cdt_thresh      = 15;
#ifdef BUILDSYS_FRAMEWORK_CASSINI
			core_mac->settings.tx_cdt_init_val    = 64;
#else /* rosetta */
			core_mac->settings.tx_cdt_init_val    = 72;
#endif /* BUILDSYS_FRAMEWORK_CASSINI */
			core_mac->settings.tx_pcs_credits     = 8;
			core_mac->settings.llr_if_credits     = 8;
			break;
		case SL_MEDIA_FURCATION_X2:
			core_mac->settings.tx_idle_delay      = 0;
			core_mac->settings.tx_priority_thresh = 32;
			core_mac->settings.tx_cdt_thresh_2    = 32;
			core_mac->settings.tx_cdt_thresh      = 15;
			core_mac->settings.tx_cdt_init_val    = 72;
			core_mac->settings.tx_pcs_credits     = 4;
			core_mac->settings.llr_if_credits     = 4;
			break;
		case SL_MEDIA_FURCATION_X4:
			core_mac->settings.tx_idle_delay      = 1;
			core_mac->settings.tx_priority_thresh = 23;
			core_mac->settings.tx_cdt_thresh_2    = 20;
			core_mac->settings.tx_cdt_thresh      = 12;
			core_mac->settings.tx_cdt_init_val    = 40;
			core_mac->settings.tx_pcs_credits     = 2;
			core_mac->settings.llr_if_credits     = 2;
			break;
		}
		core_mac->settings.tx_short_preamble = 0;
	}

	if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_8BYTE_PREAMBLE))
		core_mac->settings.tx_short_preamble = 0;

	return 0;
}

int sl_core_data_mac_rx_settings(struct sl_core_mac *core_mac)
{
	struct sl_lgrp_config *lgrp_config;

	sl_core_log_dbg(core_mac, LOG_NAME, "rx settings");

	lgrp_config = &(core_mac->core_lgrp->config);

	if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_FABRIC)) {
		core_mac->settings.rx_flit_packing_cnt = 1;
		core_mac->settings.rx_short_preamble   = 1;
	} else {
		core_mac->settings.rx_flit_packing_cnt = 1;
		core_mac->settings.rx_short_preamble   = 0;
	}

	if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_8BYTE_PREAMBLE))
		core_mac->settings.rx_short_preamble   = 0;

	return 0;
}

u32 sl_core_data_mac_tx_state_get(struct sl_core_mac *core_mac)
{
	switch (sl_core_hw_mac_tx_state_get(core_mac)) {
	case 0:
		sl_core_log_dbg(core_mac, LOG_NAME, "get TX OFF");
		return SL_CORE_MAC_STATE_OFF;
	case 1:
		sl_core_log_dbg(core_mac, LOG_NAME, "get TX ON");
		return SL_CORE_MAC_STATE_ON;
	}

	return SL_CORE_MAC_STATE_INVALID;
}

u32 sl_core_data_mac_rx_state_get(struct sl_core_mac *core_mac)
{
	switch (sl_core_hw_mac_rx_state_get(core_mac)) {
	case 0:
		sl_core_log_dbg(core_mac, LOG_NAME, "get RX OFF");
		return SL_CORE_MAC_STATE_OFF;
	case 1:
		sl_core_log_dbg(core_mac, LOG_NAME, "get RX ON");
		return SL_CORE_MAC_STATE_ON;
	}

	return SL_CORE_MAC_STATE_INVALID;
}

void sl_core_data_mac_info_map_clr(struct sl_core_mac *core_mac, u32 bit_num)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_mac->data_lock, irq_flags);

	if (bit_num == SL_CORE_INFO_MAP_NUM_BITS)
		bitmap_zero((unsigned long *)&(core_mac->info_map), SL_CORE_INFO_MAP_NUM_BITS);
	else
		clear_bit(bit_num, (unsigned long *)&(core_mac->info_map));

	sl_core_log_dbg(core_mac, LOG_NAME,
		"clr info map 0x%016llX", core_mac->info_map);

	spin_unlock_irqrestore(&core_mac->data_lock, irq_flags);
}

void sl_core_data_mac_info_map_set(struct sl_core_mac *core_mac, u32 bit_num)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_mac->data_lock, irq_flags);
	set_bit(bit_num, (unsigned long *)&(core_mac->info_map));

	sl_core_log_dbg(core_mac, LOG_NAME,
		"set info map 0x%016llX", core_mac->info_map);

	spin_unlock_irqrestore(&core_mac->data_lock, irq_flags);
}

u64 sl_core_data_mac_info_map_get(struct sl_core_mac *core_mac)
{
	unsigned long irq_flags;
	u64           info_map;

	spin_lock_irqsave(&core_mac->data_lock, irq_flags);
	info_map = core_mac->info_map;
	spin_unlock_irqrestore(&core_mac->data_lock, irq_flags);

	sl_core_log_dbg(core_mac, LOG_NAME,
		"get info map 0x%016llX", info_map);

	return info_map;
}
