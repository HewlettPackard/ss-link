// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_lgrp.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_llr.h"

#define LOG_NAME SL_CORE_HW_LLR_LOG_NAME

void sl_core_hw_llr_config_timeouts(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "config timeouts (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_0(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_0_PCS_LINK_DN_TIMER_MAX_UPDATE(data64, 0x389ACA00);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_0(core_llr->num), data64);
	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_1(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_1_DATA_AGE_TIMER_MAX_UPDATE(data64, 0xEE6B2800);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_1(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_1(core_llr->num));
}
