// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "sl_kconfig.h"

#include "data/sl_core_data_link.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_log.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_an_lp.h"

#define LOG_NAME SL_CORE_LINK_AN_LOG_NAME

int sl_core_link_an_lp_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	sl_core_link_an_callback_t callback, void *tag, struct sl_link_caps *caps,
	u32 timeout_ms, u32 flags)
{
	unsigned long        irq_flags;
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps get");

	spin_lock_irqsave(&core_link->link.data_lock, irq_flags);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		core_link->link.state = SL_CORE_LINK_STATE_AN;
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		sl_core_hw_an_lp_caps_get_cmd(core_link, link_state, callback, tag, caps, timeout_ms, flags);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"lp caps get - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock_irqrestore(&core_link->link.data_lock, irq_flags);
		return -EBADRQC;
	}
}

u32 sl_core_link_an_lp_caps_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_data_link_an_lp_caps_state_get(sl_core_link_get(ldev_num, lgrp_num, link_num));
}

int sl_core_link_an_lp_caps_stop(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps stop");

	link_state = sl_core_data_link_state_get(core_link);

	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		return 0;
	case SL_CORE_LINK_STATE_AN:
		sl_core_hw_an_lp_caps_stop_cmd(core_link);
		return 0;
	case SL_CORE_LINK_STATE_UP:
		sl_core_log_dbg(core_link, LOG_NAME, "lp caps stop - link is up");
		return 0;
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_GOING_DOWN:
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "lp caps stop - in transition");
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"lp caps stop - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		return -EBADRQC;
	}
}

void sl_core_link_an_lp_caps_free(u8 ldev_num, u8 lgrp_num, u8 link_num,
	struct sl_link_caps *caps)
{
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps free");

	kmem_cache_free(core_link->an.lp_caps_cache, caps);
}
