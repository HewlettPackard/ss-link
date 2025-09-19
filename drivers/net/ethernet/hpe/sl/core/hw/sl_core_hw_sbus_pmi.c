// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus_pmi.h"

#define LOG_NAME SL_CORE_HW_SBUS_PMI_LOG_NAME

int sbus_pmi_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 dev_id, u8 lane,
		u8 sel, u16 addr, u16 mask, u16 *data)
{
	int rtn;
	u32 addr32;
	u32 data32;

	addr32 = PMI_ADDR32(dev_id, sel, lane, addr);

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, 2, addr32); /* PMI_ADDR */
	if (rtn) {
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS PMI RD (dev = 0x%02X, id = 0x%02X, addr = 0x%04X, mask = 0x%04X)",
			dev_addr, dev_id, addr, mask);
		return rtn;
	}

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, 4, &data32); /* PMI DATA OUT */
	if (rtn) {
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS PMI RD (dev = 0x%02X, id = 0x%02X, addr = 0x%04X, mask = 0x%04X)",
			dev_addr, dev_id, addr, mask);
		return rtn;
	}

	if (data32 & BIT(17)) { /* PMI ERROR */
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS PMI RD (dev = 0x%02X, id = 0x%02X, addr = 0x%04X, mask = 0x%04X)",
			dev_addr, dev_id, addr, mask);
		return -EIO;
	}

	*data = (u16)data32 & mask;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"SBUS PMI RD (dev = 0x%02X, addr = 0x%08X, data = 0x%08X)",
		dev_addr, addr32, *data);

	return 0;
}

int sbus_pmi_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 dev_id, u8 lane,
		u8 sel, u16 addr, u16 data, u16 mask)
{
	int rtn;
	u32 addr32;
	u32 data32;

	addr32 = PMI_ADDR32(dev_id, sel, lane, addr);
	data32 = (~mask & 0xFFFF) | (data << 16);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"SBUS PMI WR (dev = 0x%02X, addr = 0x%08X, data = 0x%08X)",
		dev_addr, addr32, data32);

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, 2, addr32); /* PMI_ADDR */
	if (rtn) {
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS PMI WR (dev = 0x%02X, id = 0x%02X, addr = 0x%04X, data = 0x%04X, mask = 0x%04X)",
			dev_addr, dev_id, addr, data, mask);
		return rtn;
	}

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, 3, data32); /* PMI MASK+DATA */
	if (rtn != 0) {
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS PMI WR (dev = 0x%02X, id = 0x%02X, addr = 0x%04X, data = 0x%04X, mask = 0x%04X)",
			dev_addr, dev_id, addr, data, mask);
		return rtn;
	}

	return 0;
}
