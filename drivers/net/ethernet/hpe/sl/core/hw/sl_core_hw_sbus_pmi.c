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

#define SL_CORE_HW_SBUS_PMI_ADDR_REG      2
#define SL_CORE_HW_SBUS_PMI_DATA_MASK_REG 3
#define SL_CORE_HW_SBUS_PMI_DATA_OUT_REG  4

int sbus_pmi_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 dev_id, u8 lane,
		u8 pll, u16 addr, u16 mask, u16 *data)
{
	int rtn;
	u32 addr32;
	u32 data32;

	addr32 = PMI_ADDR32(dev_id, pll, lane, addr);

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, SL_CORE_HW_SBUS_PMI_ADDR_REG, addr32); /* PMI_ADDR */
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
				      "SBUS PMI RD (dev = 0x%02X, dev_id = 0x%02X, addr = 0x%04X, mask = 0x%04X)",
				      dev_addr, dev_id, addr, mask);
		goto out;
	}

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, SL_CORE_HW_SBUS_PMI_DATA_OUT_REG, &data32); /* PMI DATA OUT */
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
				      "SBUS PMI RD (dev = 0x%02X, dev_id = 0x%02X, addr = 0x%04X, mask = 0x%04X)",
				      dev_addr, dev_id, addr, mask);
		goto out;
	}

	if (data32 & BIT(17)) { /* PMI ERROR */
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
				      "SBUS PMI RD (dev = 0x%02X, dev_id = 0x%02X, addr = 0x%04X, mask = 0x%04X)",
				      dev_addr, dev_id, addr, mask);
		rtn = -EIO;
		goto out;
	}

	*data = (u16)data32 & mask;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "SBUS PMI RD (dev = 0x%02X, addr = 0x%08X, data = 0x%08X)",
			dev_addr, addr32, *data);

out:
	spin_lock(&core_lgrp->data_lock);
	core_lgrp->sbus_pmi.rd.dev_addr =  dev_addr;
	core_lgrp->sbus_pmi.rd.addr     =  addr;
	core_lgrp->sbus_pmi.rd.data     = *data;
	core_lgrp->sbus_pmi.rd.mask     =  mask;
	core_lgrp->sbus_pmi.rd.dev_id   =  dev_id;
	core_lgrp->sbus_pmi.rd.lane     =  lane;
	core_lgrp->sbus_pmi.rd.pll      =  pll;
	core_lgrp->sbus_pmi.rd.result   =  rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}

int sbus_pmi_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 dev_id, u8 lane,
		u8 pll, u16 addr, u16 data, u16 mask)
{
	int rtn;
	u32 addr32;
	u32 data32;

	addr32 = PMI_ADDR32(dev_id, pll, lane, addr);
	data32 = (~mask & 0xFFFF) | (data << 16);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "SBUS PMI WR (dev = 0x%02X, addr = 0x%08X, data = 0x%08X)",
			dev_addr, addr32, data32);

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, SL_CORE_HW_SBUS_PMI_ADDR_REG, addr32); /* PMI_ADDR */
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
				      "SBUS PMI WR (dev = 0x%02X, dev_id = 0x%02X, addr = 0x%04X, data = 0x%04X, mask = 0x%04X)",
				      dev_addr, dev_id, addr, data, mask);
		goto out;
	}

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, SL_CORE_HW_SBUS_PMI_DATA_MASK_REG, data32); /* PMI MASK+DATA */
	if (rtn != 0)
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
				      "SBUS PMI WR (dev = 0x%02X, dev_id = 0x%02X, addr = 0x%04X, data = 0x%04X, mask = 0x%04X)",
				      dev_addr, dev_id, addr, data, mask);

out:
	spin_lock(&core_lgrp->data_lock);
	core_lgrp->sbus_pmi.wr.dev_addr = dev_addr;
	core_lgrp->sbus_pmi.wr.addr     = addr;
	core_lgrp->sbus_pmi.wr.data     = data;
	core_lgrp->sbus_pmi.wr.mask     = mask;
	core_lgrp->sbus_pmi.wr.dev_id   = dev_id;
	core_lgrp->sbus_pmi.wr.lane     = lane;
	core_lgrp->sbus_pmi.wr.pll      = pll;
	core_lgrp->sbus_pmi.wr.result   = rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}
