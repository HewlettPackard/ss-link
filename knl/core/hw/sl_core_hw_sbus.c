// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

#include "sl_kconfig.h"
#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_sbus.h"

#define LOG_NAME SL_CORE_HW_SBUS_LOG_NAME

int sbus_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u32 data)
{
	int rtn;

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, reg, data);
	if (rtn != 0)
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS WR (dev = 0x%02X, reg = 0x%02X, data = 0x%08X)",
			dev_addr, reg, data);

	return rtn;
}

int sbus_field_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u8 lsb, u32 mask, u32 data)
{
	int rtn;
	u32 data32;

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, reg, &data32);
	if (rtn != 0) {
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS field RD (dev = 0x%02X, reg = 0x%02X)",
			dev_addr, reg);
		return rtn;
	}

	data32 = ((data32 & (~(mask << lsb))) | (data << lsb));

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, reg, data32);
	if (rtn != 0)
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS field WR (dev = 0x%02X, reg = 0x%02X, lsb = %u, data = 0x%08X)",
			dev_addr, reg, lsb, data32);

	return rtn;
}

int sbus_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u32 *data)
{
	int rtn;

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, reg, data);
	if (rtn != 0)
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS RD (dev = 0x%02X, reg = 0x%02X)",
			dev_addr, reg);

	return rtn;
}

int sbus_field_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u8 lsb, u32 mask, u32 *data)
{
	int rtn;
	u32 data32;

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, reg, &data32);
	if (rtn != 0)
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS field RD (dev = 0x%02X, reg = 0x%02X, lsb = %u)",
			dev_addr, reg, lsb);

	*data = ((data32 >> lsb) & mask);

	return rtn;
}

int sbus_rst(struct sl_core_lgrp *core_lgrp, u8 dev_addr)
{
	int rtn;

	rtn = sl_core_sbus_rst(core_lgrp, dev_addr);
	if (rtn != 0)
		sl_core_log_err(core_lgrp, LOG_NAME,
			"SBUS RST (dev = 0x%02X)", dev_addr);

	return rtn;
}
