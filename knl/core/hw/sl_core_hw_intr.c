// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_work_link.h"
#include "base/sl_core_log.h"
#include "data/sl_core_data_ldev.h"
#include "data/sl_core_data_lgrp.h"
#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_an.h"

#define LOG_NAME SL_CORE_HW_INTR_LOG_NAME

static void sl_core_hw_intr_flgs_clr_source(struct sl_core_link *core_link, u32 intr_num)
{
	int x;
	u64 addr;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
		"flgs clr source (port = %d, intr = %d, "
		"flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port, intr_num,
		core_link->intrs[intr_num].source[0],
		core_link->intrs[intr_num].source[1],
		core_link->intrs[intr_num].source[2],
		core_link->intrs[intr_num].source[3]);

	addr = SS2_PORT_PML_ERR_CLR;
	for (x = 0; x < SL_CORE_HW_INTR_FLGS_COUNT; ++x, addr += 8)
		sl_core_write64(core_link, addr, core_link->intrs[intr_num].source[x]);
	sl_core_flush64(core_link, SS2_PORT_PML_ERR_CLR);
}

void sl_core_hw_intr_hdlr(u64 *err_flgs, int num_err_flgs, void *data)
{
	int                          rtn;
	struct sl_core_hw_intr_data *info;
	struct sl_core_link         *core_link;
	int                          x;

	info = data;
	core_link = info->link;

	sl_core_log_dbg(core_link, LOG_NAME,
		"hdlr - %s (link = 0x%p, intr = %u, work = %u)",
		info->log, core_link, info->intr_num, info->work_num);

	rtn = sl_core_hw_intr_flgs_disable(core_link, info->intr_num);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"hdlr - %s - flgs disable failed [%d]",
			info->log, rtn);

	for (x = 0; x < SL_CORE_HW_INTR_FLGS_COUNT; ++x)
		core_link->intrs[info->intr_num].source[x] |= err_flgs[x];

	sl_core_hw_intr_flgs_clr_source(core_link, info->intr_num);

	queue_work(core_link->core_lgrp->core_ldev->workqueue, &(core_link->work[info->work_num]));
}

int sl_core_hw_intr_hdlr_register(struct sl_core_link *core_link)
{
	int rtn;
	int x;

	sl_core_log_dbg(core_link, LOG_NAME, "register");

	for (x = 0; x < SL_CORE_HW_INTR_COUNT; ++x) {
		if (x == SL_CORE_HW_INTR_AN_PAGE_RECV)
			rtn = sl_core_hw_intr_register(core_link, core_link->intrs[x].flgs,
				sl_core_hw_an_intr_hdlr, &(core_link->intrs[x].data));
		else
			rtn = sl_core_hw_intr_register(core_link, core_link->intrs[x].flgs,
				sl_core_hw_intr_hdlr, &(core_link->intrs[x].data));
		if (rtn != 0) {
			sl_core_log_err(core_link, LOG_NAME,
				"register - %d failed [%d]", x, rtn);
			return rtn;
		}
	}

	return 0;
}

void sl_core_hw_intr_hdlr_unregister(struct sl_core_link *core_link)
{
	int rtn;
	int x;

	sl_core_log_dbg(core_link, LOG_NAME, "unregister");

	for (x = 0; x < SL_CORE_HW_INTR_COUNT; ++x) {
		if (x == SL_CORE_HW_INTR_AN_PAGE_RECV)
			rtn = sl_core_hw_intr_unregister(core_link, core_link->intrs[x].flgs,
				sl_core_hw_an_intr_hdlr);
		else
			rtn = sl_core_hw_intr_unregister(core_link, core_link->intrs[x].flgs,
				sl_core_hw_intr_hdlr);
		if (rtn != 0)
			sl_core_log_warn_trace(core_link, LOG_NAME,
				"unregister - %d failed [%d]", x, rtn);
	}
}

void sl_core_hw_intr_flgs_check(struct sl_core_link *core_link, u32 intr_num)
{
	int x;
	u64 addr;
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "flgs check (port = %d)", port);

	addr = SS2_PORT_PML_ERR_FLG;
	for (x = 0; x < SL_CORE_HW_INTR_FLGS_COUNT; ++x, addr += 8) {
		sl_core_read64(core_link, addr, &data64);
		if ((data64 & core_link->intrs[intr_num].flgs[x]) != 0) {
			sl_core_log_dbg(core_link, LOG_NAME,
				"flgs check (intr = %d, 0x%016llX = 0x%016llX)",
				intr_num, addr, data64);
		}
	}
}

void sl_core_hw_intr_flgs_clr(struct sl_core_link *core_link, u32 intr_num)
{
	int x;
	u64 addr;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
		"flgs clr (port = %d, intr = %d, "
		"flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port, intr_num,
		core_link->intrs[intr_num].flgs[0],
		core_link->intrs[intr_num].flgs[1],
		core_link->intrs[intr_num].flgs[2],
		core_link->intrs[intr_num].flgs[3]);

	addr = SS2_PORT_PML_ERR_CLR;
	for (x = 0; x < SL_CORE_HW_INTR_FLGS_COUNT; ++x, addr += 8)
		sl_core_write64(core_link, addr, core_link->intrs[intr_num].flgs[x]);
	sl_core_flush64(core_link, SS2_PORT_PML_ERR_CLR);
}

int sl_core_hw_intr_flgs_enable(struct sl_core_link *core_link, u32 intr_num)
{
	int rtn;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
		"flgs enable (port = %d, intr = %d, "
		"flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port, intr_num,
		core_link->intrs[intr_num].flgs[0],
		core_link->intrs[intr_num].flgs[1],
		core_link->intrs[intr_num].flgs[2],
		core_link->intrs[intr_num].flgs[3]);

	memset(&(core_link->intrs[intr_num].source), 0, sizeof(core_link->intrs[intr_num].source));

	if (intr_num == SL_CORE_HW_INTR_AN_PAGE_RECV)
		rtn = sl_core_hw_intr_enable(core_link,
			core_link->intrs[intr_num].flgs, sl_core_hw_an_intr_hdlr);
	else
		rtn = sl_core_hw_intr_enable(core_link,
			core_link->intrs[intr_num].flgs, sl_core_hw_intr_hdlr);
	if (rtn != 0) {
		if (rtn != -EALREADY)
			sl_core_log_err_trace(core_link, LOG_NAME,
				"flgs enable failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_core_hw_intr_flgs_disable(struct sl_core_link *core_link, u32 intr_num)
{
	int rtn;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
		"flgs disable (port = %d, intr = %d, "
		"flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port, intr_num,
		core_link->intrs[intr_num].flgs[0],
		core_link->intrs[intr_num].flgs[1],
		core_link->intrs[intr_num].flgs[2],
		core_link->intrs[intr_num].flgs[3]);

	if (intr_num == SL_CORE_HW_INTR_AN_PAGE_RECV)
		rtn = sl_core_hw_intr_disable(core_link,
			core_link->intrs[intr_num].flgs, sl_core_hw_an_intr_hdlr);
	else
		rtn = sl_core_hw_intr_disable(core_link,
			core_link->intrs[intr_num].flgs, sl_core_hw_intr_hdlr);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"flgs disable failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
