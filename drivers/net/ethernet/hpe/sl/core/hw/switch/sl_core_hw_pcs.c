// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pcs.h"

#define LOG_NAME SL_CORE_HW_PCS_LOG_NAME

void sl_core_hw_pcs_keep_all_lanes_active_set(struct sl_core_link *core_link, u64 value)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
			"keep all lanes active set (port = %u, value 0x%llX)", port, value);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_TX_PCS, &data64);
	data64 = SS2_PORT_PML_CFG_TX_PCS_KEEP_ALL_LANES_ACTIVE_UPDATE(data64, value);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_TX_PCS, data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_TX_PCS);
}
