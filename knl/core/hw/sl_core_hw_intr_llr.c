// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"

#include "sl_core_llr.h"
#include "base/sl_core_work_llr.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_intr_llr.h"

#define LOG_NAME SL_CORE_HW_INTR_LLR_LOG_NAME

void sl_core_hw_intr_llr_hdlr(u64 *err_flgs, int num_err_flgs, void *data)
{
	int                              rtn;
	struct sl_core_hw_intr_llr_data *info;
	struct sl_core_llr              *core_llr;

	info = data;
	core_llr = info->core_llr;

	sl_core_log_dbg(core_llr, LOG_NAME,
		"hdlr - %s (llr = 0x%p, intr = %d, work = %d)",
		info->log, core_llr, info->intr_num, info->work_num);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, info->intr_num);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"hdlr - %s - flgs disable failed [%d]",
			info->log, rtn);

	memcpy(&(core_llr->intrs[info->intr_num].source), err_flgs,
		sizeof(core_llr->intrs[info->intr_num].source));

	sl_core_work_llr_queue(core_llr, info->work_num);
}

int sl_core_hw_intr_llr_hdlr_register(struct sl_core_llr *core_llr)
{
	int rtn;
	int x;

	sl_core_log_dbg(core_llr, LOG_NAME, "register");

	for (x = 0; x < SL_CORE_HW_INTR_LLR_COUNT; ++x) {
		rtn = sl_core_hw_intr_llr_register(core_llr, core_llr->intrs[x].flgs,
			sl_core_hw_intr_llr_hdlr, &(core_llr->intrs[x].data));
		if (rtn != 0) {
			sl_core_log_err(core_llr, LOG_NAME,
				"register - %d failed [%d]", x, rtn);
			return rtn;
		}
	}

	return 0;
}

void sl_core_hw_intr_llr_hdlr_unregister(struct sl_core_llr *core_llr)
{
	int rtn;
	int x;

	sl_core_log_dbg(core_llr, LOG_NAME, "unregister");

	for (x = 0; x < SL_CORE_HW_INTR_LLR_COUNT; ++x) {
		rtn = sl_core_hw_intr_llr_unregister(core_llr, core_llr->intrs[x].flgs,
			sl_core_hw_intr_llr_hdlr);
		if (rtn != 0)
			sl_core_log_warn_trace(core_llr, LOG_NAME,
				"unregister - %d failed [%d]", x, rtn);
	}
}

void sl_core_hw_intr_llr_flgs_check(struct sl_core_llr *core_llr, u32 intr_num)
{
	int x;
	u64 addr;
	u64 data64;
	u32 port;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "flgs check (port = %d)", port);

	addr = SS2_PORT_PML_ERR_FLG;
	for (x = 0; x < SL_CORE_HW_INTR_LLR_FLGS_COUNT; ++x, addr += 8) {
		sl_core_llr_read64(core_llr, addr, &data64);
		if ((data64 & core_llr->intrs[intr_num].flgs[x]) != 0) {
			sl_core_log_dbg(core_llr, LOG_NAME,
				"flgs check (intr = %d, 0x%016llX = 0x%016llX)",
				intr_num, addr, data64);
		}
	}
}

void sl_core_hw_intr_llr_flgs_clr(struct sl_core_llr *core_llr, u32 intr_num)
{
	int x;
	u64 addr;
	u32 port;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME,
		"flgs clr (port = %d, intr = %d, "
		"flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port, intr_num,
		core_llr->intrs[intr_num].flgs[0],
		core_llr->intrs[intr_num].flgs[1],
		core_llr->intrs[intr_num].flgs[2],
		core_llr->intrs[intr_num].flgs[3]);

	addr = SS2_PORT_PML_ERR_CLR;
	for (x = 0; x < SL_CORE_HW_INTR_LLR_FLGS_COUNT; ++x, addr += 8)
		sl_core_llr_write64(core_llr, addr, core_llr->intrs[intr_num].flgs[x]);
	sl_core_llr_flush64(core_llr, SS2_PORT_PML_ERR_CLR);
}

int sl_core_hw_intr_llr_flgs_enable(struct sl_core_llr *core_llr, u32 intr_num)
{
	int rtn;
	u32 port;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME,
		"flgs enable (port = %d, intr = %d, "
		"flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port, intr_num,
		core_llr->intrs[intr_num].flgs[0],
		core_llr->intrs[intr_num].flgs[1],
		core_llr->intrs[intr_num].flgs[2],
		core_llr->intrs[intr_num].flgs[3]);

	rtn = sl_core_hw_intr_llr_enable(core_llr,
		core_llr->intrs[intr_num].flgs, sl_core_hw_intr_llr_hdlr);
	if (rtn != 0) {
		if (rtn != -EALREADY)
			sl_core_log_err_trace(core_llr, LOG_NAME,
				"flgs enable failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_core_hw_intr_llr_flgs_disable(struct sl_core_llr *core_llr, u32 intr_num)
{
	int rtn;
	u32 port;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME,
		"flgs disable (port = %d, intr = %d, "
		"flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port, intr_num,
		core_llr->intrs[intr_num].flgs[0],
		core_llr->intrs[intr_num].flgs[1],
		core_llr->intrs[intr_num].flgs[2],
		core_llr->intrs[intr_num].flgs[3]);

	rtn = sl_core_hw_intr_llr_disable(core_llr,
		core_llr->intrs[intr_num].flgs, sl_core_hw_intr_llr_hdlr);
	if (rtn != 0) {
		sl_core_log_err_trace(core_llr, LOG_NAME,
			"flgs disable failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
