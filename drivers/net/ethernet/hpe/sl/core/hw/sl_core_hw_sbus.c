// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

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
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "SBUS WR (dev = 0x%02X, reg = 0x%02X, data = 0x%08X)",
				      dev_addr, reg, data);

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->sbus.wr.dev_addr = dev_addr;
	core_lgrp->sbus.wr.data     = data;
	core_lgrp->sbus.wr.mask     = 0xFFFFFFFF;
	core_lgrp->sbus.wr.reg      = reg;
	core_lgrp->sbus.wr.lsb      = 0;
	core_lgrp->sbus.wr.result   = rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}

int sbus_field_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u8 lsb, u32 mask, u32 data)
{
	int rtn;
	u32 data32;

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, reg, &data32);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "SBUS field RD (dev = 0x%02X, reg = 0x%02X)", dev_addr, reg);
		goto out;
	}

	data32 = ((data32 & (~(mask << lsb))) | (data << lsb));

	rtn = sl_core_sbus_wr(core_lgrp, dev_addr, reg, data32);
	if (rtn != 0)
		sl_core_log_err_trace(core_lgrp, LOG_NAME,
				      "SBUS field WR (dev = 0x%02X, reg = 0x%02X, lsb = %u, data = 0x%08X)",
				      dev_addr, reg, lsb, data32);

out:
	spin_lock(&core_lgrp->data_lock);
	core_lgrp->sbus.wr.dev_addr = dev_addr;
	core_lgrp->sbus.wr.data     = data;
	core_lgrp->sbus.wr.mask     = mask;
	core_lgrp->sbus.wr.reg      = reg;
	core_lgrp->sbus.wr.lsb      = lsb;
	core_lgrp->sbus.wr.result   = rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}

int sbus_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u32 *data)
{
	int rtn;

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, reg, data);
	if (rtn != 0)
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "SBUS RD (dev = 0x%02X, reg = 0x%02X)", dev_addr, reg);

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->sbus.rd.dev_addr =  dev_addr;
	core_lgrp->sbus.rd.data     = *data;
	core_lgrp->sbus.rd.mask     =  0xFFFFFFFF;
	core_lgrp->sbus.rd.reg      =  reg;
	core_lgrp->sbus.rd.lsb      =  0;
	core_lgrp->sbus.rd.result   =  rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}

int sbus_field_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u8 lsb, u32 mask, u32 *data)
{
	int rtn;
	u32 data32;

	rtn = sl_core_sbus_rd(core_lgrp, dev_addr, reg, &data32);
	if (rtn != 0)
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "SBUS field RD (dev = 0x%02X, reg = 0x%02X, lsb = %u)",
				      dev_addr, reg, lsb);

	*data = ((data32 >> lsb) & mask);

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->sbus.rd.dev_addr =  dev_addr;
	core_lgrp->sbus.rd.data     = *data;
	core_lgrp->sbus.rd.mask     =  mask;
	core_lgrp->sbus.rd.reg      =  reg;
	core_lgrp->sbus.rd.lsb      =  lsb;
	core_lgrp->sbus.rd.result   =  rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}

int sbus_rst(struct sl_core_lgrp *core_lgrp, u8 dev_addr)
{
	int rtn;

	rtn = sl_core_sbus_rst(core_lgrp, dev_addr);
	if (rtn != 0)
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "SBUS RST (dev = 0x%02X)", dev_addr);

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->sbus.rst.dev_addr = dev_addr;
	core_lgrp->sbus.rst.result   = rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}
