// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pmi.h"

#define LOG_NAME SL_CORE_HW_PMI_LOG_NAME

int pmi_rd(struct sl_core_lgrp *core_lgrp, u8 dev_id, u8 lane, u8 pll, u16 addr, u16 *data)
{
	int rtn;

	rtn = sl_core_pmi_rd(core_lgrp, PMI_ADDR32(dev_id, pll, lane, addr), data);
	if (rtn)
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "PMI RD (addr = 0x%04X, data = 0x%04X)", addr, *data);

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->pmi.rd.addr   =  addr;
	core_lgrp->pmi.rd.data   = *data;
	core_lgrp->pmi.rd.dev_id =  dev_id;
	core_lgrp->pmi.rd.lane   =  lane;
	core_lgrp->pmi.rd.pll    =  pll;
	core_lgrp->pmi.rd.result =  rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}

int pmi_wr(struct sl_core_lgrp *core_lgrp, u8 dev_id, u8 lane, u8 pll, u16 addr, u16 data, u16 mask)
{
	int rtn;

	rtn = sl_core_pmi_wr(core_lgrp, PMI_ADDR32(dev_id, pll, lane, addr), data, mask);
	if (rtn)
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "PMI WR (addr = 0x%04X, data = 0x%04X, mask = 0x%04X)",
				      addr, data, mask);

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->pmi.wr.addr   = addr;
	core_lgrp->pmi.wr.data   = data;
	core_lgrp->pmi.wr.mask   = mask;
	core_lgrp->pmi.wr.dev_id = dev_id;
	core_lgrp->pmi.wr.lane   = lane;
	core_lgrp->pmi.wr.pll    = pll;
	core_lgrp->pmi.wr.result = rtn;
	spin_unlock(&core_lgrp->data_lock);

	return rtn;
}
