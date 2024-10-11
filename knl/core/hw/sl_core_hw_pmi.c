// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

#include "sl_kconfig.h"
#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pmi.h"

#define LOG_NAME SL_CORE_HW_PMI_LOG_NAME

#define DEV_ID_MASK  0x1F
#define DEV_ID_SHIFT 27
#define PLL_MASK     0x07
#define PLL_SHIFT    24
#define LANE_MASK    0xFF
#define LANE_SHIFT   16
#define ADDR_MASK    0xFFFF

#define ADDR32(_dev_id, _pll, _lane, _addr)          \
	(((_dev_id & DEV_ID_MASK) << DEV_ID_SHIFT) | \
	 ((_pll    & PLL_MASK)    << PLL_SHIFT)    | \
	 ((_lane   & LANE_MASK)   << LANE_SHIFT)   | \
	  (_addr   & ADDR_MASK))

int pmi_rd(struct sl_core_lgrp *core_lgrp, u8 dev_id, u8 lane,
	u8 pll, u16 addr, u8 shl, u8 shr, u16 *data)
{
	int rtn;
	u16 data16;

	rtn = sl_core_pmi_rd(core_lgrp, ADDR32(dev_id, pll, lane, addr), &data16);
	if (rtn != 0)
		sl_core_log_err(core_lgrp, LOG_NAME,
			"PMI RD (addr = 0x%04X, shl = %u, shr = %u)",
			addr, shl, shr);

	data16 = data16 << shl;
	data16 = data16 >> shr;
	*data  = (data16 & 0xFFFF);

	return rtn;
}

int pmi_wr(struct sl_core_lgrp *core_lgrp, u8 dev_id, u8 lane,
	u8 pll, u16 addr, u16 data, u8 shl, u16 mask)
{
	int rtn;

	rtn = sl_core_pmi_wr(core_lgrp, ADDR32(dev_id, pll, lane, addr), (data << shl), mask);
	if (rtn != 0)
		sl_core_log_err(core_lgrp, LOG_NAME,
			"PMI WR (addr = 0x%04X, data = 0x%04X, shl = %u, mask = 0x%04X)",
			addr, data, shl, mask);

	return rtn;
}
