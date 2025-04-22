// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/bitops.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "data/sl_core_data_ldev.h"
#include "data/sl_core_data_lgrp.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_reset.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_serdes_lane.h"

static struct sl_core_lgrp *core_lgrps[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS];
static DEFINE_SPINLOCK(core_lgrps_lock);

#define LOG_NAME SL_CORE_DATA_LGRP_LOG_NAME

int sl_core_data_lgrp_new(u8 ldev_num, u8 lgrp_num)
{
	int                  rtn;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
	if (core_lgrp) {
		sl_core_log_err(core_lgrp, LOG_NAME, "new - exists (lgrp = 0x%p)", core_lgrp);
		return -EBADRQC;
	}

	core_lgrp = kzalloc(sizeof(struct sl_core_lgrp), GFP_KERNEL);
	if (core_lgrp == NULL)
		return -ENOMEM;

	core_lgrp->magic     = SL_CORE_LGRP_MAGIC;
	core_lgrp->core_ldev = sl_core_data_ldev_get(ldev_num);
	core_lgrp->num       = lgrp_num;
	snprintf(core_lgrp->log_connect_id, sizeof(core_lgrp->log_connect_id), "core-lgrp%02u", lgrp_num);
	spin_lock_init(&(core_lgrp->log_lock));
	spin_lock_init(&(core_lgrp->data_lock));

	if (SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev)) {
		rtn = core_lgrp->core_ldev->ops.dt_info_get(core_lgrp->core_ldev->accessors.dt,
			ldev_num, lgrp_num, &(core_lgrp->serdes.dt));
		if (rtn)
			sl_core_log_warn(core_lgrp, LOG_NAME, "new - dt_info_get failed [%d}", rtn);
		sl_core_log_dbg(core_lgrp, LOG_NAME, "DT lane = 0 tx = %u tx_inv = %u rx = %u rx_inv = %u",
			core_lgrp->serdes.dt.lane_info[0].tx_source, core_lgrp->serdes.dt.lane_info[0].tx_invert,
			core_lgrp->serdes.dt.lane_info[0].rx_source, core_lgrp->serdes.dt.lane_info[0].rx_invert);
		sl_core_log_dbg(core_lgrp, LOG_NAME, "DT lane = 1 tx = %u tx_inv = %u rx = %u rx_inv = %u",
			core_lgrp->serdes.dt.lane_info[1].tx_source, core_lgrp->serdes.dt.lane_info[1].tx_invert,
			core_lgrp->serdes.dt.lane_info[1].rx_source, core_lgrp->serdes.dt.lane_info[1].rx_invert);
		sl_core_log_dbg(core_lgrp, LOG_NAME, "DT lane = 2 tx = %u tx_inv = %u rx = %u rx_inv = %u",
			core_lgrp->serdes.dt.lane_info[2].tx_source, core_lgrp->serdes.dt.lane_info[2].tx_invert,
			core_lgrp->serdes.dt.lane_info[2].rx_source, core_lgrp->serdes.dt.lane_info[2].rx_invert);
		sl_core_log_dbg(core_lgrp, LOG_NAME, "DT lane = 3 tx = %u tx_inv = %u rx = %u rx_inv = %u",
			core_lgrp->serdes.dt.lane_info[3].tx_source, core_lgrp->serdes.dt.lane_info[3].tx_invert,
			core_lgrp->serdes.dt.lane_info[3].rx_source, core_lgrp->serdes.dt.lane_info[3].rx_invert);
	} else {
		/* straight for emulator */
		core_lgrp->serdes.dt.lane_info[0].tx_source = 0;
		core_lgrp->serdes.dt.lane_info[1].tx_source = 1;
		core_lgrp->serdes.dt.lane_info[2].tx_source = 2;
		core_lgrp->serdes.dt.lane_info[3].tx_source = 3;
		core_lgrp->serdes.dt.lane_info[0].rx_source = 0;
		core_lgrp->serdes.dt.lane_info[1].rx_source = 1;
		core_lgrp->serdes.dt.lane_info[2].rx_source = 2;
		core_lgrp->serdes.dt.lane_info[3].rx_source = 3;
	}
	sl_core_log_dbg(core_lgrp, LOG_NAME, "new - dt info (id = %u, addr = 0x%X)",
		core_lgrp->serdes.dt.dev_id, core_lgrp->serdes.dt.dev_addr);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "new (lgrp = 0x%p)", core_lgrp);

	spin_lock(&core_lgrps_lock);
	core_lgrps[ldev_num][lgrp_num] = core_lgrp;
	spin_unlock(&core_lgrps_lock);

	return 0;
}

void sl_core_data_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id)
{
	unsigned long        irq_flags;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	spin_lock_irqsave(&(core_lgrp->log_lock), irq_flags);
	strncpy(core_lgrp->log_connect_id, connect_id, SL_LOG_CONNECT_ID_LEN);
	spin_unlock_irqrestore(&(core_lgrp->log_lock), irq_flags);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "connect_id = %s", connect_id);
}

void sl_core_data_lgrp_del(u8 ldev_num, u8 lgrp_num)
{
	struct sl_core_lgrp *core_lgrp;
	u8                   link_num;
	u8                   is_configuring;

	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
	if (!core_lgrp) {
		sl_core_log_dbg(NULL, LOG_NAME, "del - not found (lgrp_num = %u)", lgrp_num);
		return;
	}

	spin_lock(&core_lgrp->data_lock);
	is_configuring = core_lgrp->is_configuring;
	spin_unlock(&core_lgrp->data_lock);
	if (is_configuring) {
		sl_core_log_err(core_lgrp, LOG_NAME, "del - lgrp is configuring");
		return;
	}

	for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num)
		sl_core_data_link_del(ldev_num, lgrp_num, link_num);

	spin_lock(&core_lgrps_lock);
	core_lgrps[ldev_num][lgrp_num] = NULL;
	spin_unlock(&core_lgrps_lock);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "del (lgrp = 0x%p)", core_lgrp);

	kfree(core_lgrp);
}

struct sl_core_lgrp *sl_core_data_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	unsigned long        irq_flags;
	struct sl_core_lgrp *core_lgrp;

	spin_lock_irqsave(&core_lgrps_lock, irq_flags);
	core_lgrp = core_lgrps[ldev_num][lgrp_num];
	spin_unlock_irqrestore(&core_lgrps_lock, irq_flags);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "get (lgrp = 0x%p)", core_lgrp);

	return core_lgrp;
}

int sl_core_data_lgrp_link_config_check(struct sl_core_lgrp *core_lgrp)
{
	int                  link_num;
	struct sl_core_link *core_link;
	u32                  link_state;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "link config check");

	for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
		core_link = sl_core_link_get(core_lgrp->core_ldev->num, core_lgrp->num, link_num);
		if (!core_link)
			continue;
		sl_core_link_state_get(core_lgrp->core_ldev->num,
			core_lgrp->num, link_num, &link_state);
		switch (link_state) {
		case SL_CORE_LINK_STATE_UNCONFIGURED:
		case SL_CORE_LINK_STATE_CONFIGURING:
		case SL_CORE_LINK_STATE_CONFIGURED:
		case SL_CORE_LINK_STATE_DOWN:
			continue;
		default:
			sl_core_log_err(core_lgrp, LOG_NAME,
				"link config check wrong state (link_num = %u, state = %u %s)",
				link_num, link_state, sl_core_link_state_str(link_state));
			return -EBADRQC;
		}
	}

	return 0;
}

void sl_core_data_lgrp_hw_attr_set(struct sl_core_lgrp *core_lgrp, struct sl_hw_attr *hw_attr)
{
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"hw attr set (cxi = %u, nic = %u)", hw_attr->cxi_num, hw_attr->nic_num);

	core_lgrp->hw_attr = *hw_attr;
}

void sl_core_data_lgrp_config_set(struct sl_core_lgrp *core_lgrp, struct sl_lgrp_config *lgrp_config)
{
	int  rtn;
	bool need_swizzled;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "config set");

	/* if furcation changes, then we have to reset */
	if (lgrp_config->furcation != core_lgrp->config.furcation)
		sl_core_hw_reset_lgrp(core_lgrp);

	/* if loopback enable changes, then we have to swizzle */
	need_swizzled = false;
	if ((lgrp_config->options & SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE) !=
		(core_lgrp->config.options & SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE))
		need_swizzled = true;

	core_lgrp->config = *lgrp_config;

	if (need_swizzled) {
		rtn = sl_core_hw_serdes_swizzles(core_lgrp);
		if (rtn)
			sl_core_log_warn_trace(core_lgrp, LOG_NAME,
				"hw_serdes_swizzles failed [%d]", rtn);
	}
}

u32 sl_core_data_lgrp_config_flags_get(struct sl_core_lgrp *core_lgrp)
{
	u32           flags;
	unsigned long irq_flags;

	spin_lock_irqsave(&core_lgrp->data_lock, irq_flags);
	flags = core_lgrp->config.options;
	spin_unlock_irqrestore(&core_lgrp->data_lock, irq_flags);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "config flags get (flags = 0x%X)", flags);

	return flags;
}
