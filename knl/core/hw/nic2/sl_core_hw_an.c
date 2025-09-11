// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_link.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_an.h"

#define LOG_NAME SL_CORE_HW_AN_LOG_NAME

void sl_core_hw_an_config_timers(struct sl_core_link *core_link)
{
	u32                                       port;
	union ss2_port_pml_cfg_pcs_autoneg_timers autoneg_timers;

	port = core_link->core_lgrp->num;
	autoneg_timers.link_fail_inhibit_timer_max = 0x3FFFFFFFF;

	sl_core_log_dbg(core_link, LOG_NAME, "config timers (port = %d)", port);

	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8, autoneg_timers.qw[1]);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8);
}
