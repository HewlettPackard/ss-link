// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025,2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_llr.h"

#define LOG_NAME SL_CORE_HW_LLR_LOG_NAME

void sl_core_hw_llr_config_timeouts(struct sl_core_llr *core_llr)
{
	u32                  port;
	u64                  data64;
	struct sl_core_link *core_link;
	bool                 is_pml_rec_enabled;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "config timeouts (port = %d)", port);

	core_link = sl_core_link_get(core_llr->core_lgrp->core_ldev->num, core_llr->core_lgrp->num, core_llr->num);

	is_pml_rec_enabled = sl_core_link_config_is_enable_pml_recovery_set(core_link);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS(core_llr->num), &data64);
	if (is_pml_rec_enabled) {
		data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_DATA_AGE_TIMER_MAX_UPDATE(data64,
										(core_link->config.pml_rec_timeout_ms + 8) * 1000000);
		data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_PCS_LINK_DN_TIMER_MAX_UPDATE(data64,
										(core_link->config.pml_rec_timeout_ms + 8) * 1000000);
	} else {
		data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_DATA_AGE_TIMER_MAX_UPDATE(data64, 0xEE6B2800);
		data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_PCS_LINK_DN_TIMER_MAX_UPDATE(data64, 0x389ACA00);
	}

	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS(core_llr->num));
}
