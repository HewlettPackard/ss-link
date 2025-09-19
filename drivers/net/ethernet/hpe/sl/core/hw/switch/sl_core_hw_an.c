// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_asic.h"
#include "sl_platform.h"
#include "base/sl_core_log.h"
#include "sl_core_link.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_an.h"

#define LOG_NAME SL_CORE_HW_AN_LOG_NAME

void sl_core_hw_an_config_timers(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "config timers (port = %d)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8, &data64);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS_WORD1_LINK_FAIL_INHIBIT_TIMER_MAX_UPDATE(data64, 0x3FFFFFFFF);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8, data64);

	if (SL_PLATFORM_IS_EMULATOR(core_link->core_lgrp->core_ldev)) {
		sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS, &data64);
		data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS_WORD0_BREAK_LINK_TIMER_MAX_UPDATE(data64, 50000);
		sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS, data64);
	}

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8);
}
