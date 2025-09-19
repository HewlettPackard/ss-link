/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_IO_H_
#define _SL_CORE_HW_IO_H_

#include <linux/hpe/sl/sl_ldev.h>

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_mac.h"
#include "sl_core_llr.h"

static inline void sl_core_lgrp_flush64(struct sl_core_lgrp *core_lgrp, u64 addr)
{
	(void)core_lgrp->core_ldev->ops.read64(
		core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"FLSH (addr = 0x%016llX)", addr);
}

static inline void sl_core_lgrp_read64(struct sl_core_lgrp *core_lgrp, u64 addr, u64 *data64)
{
	*data64 = core_lgrp->core_ldev->ops.read64(
		core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"RD64 (addr = 0x%016llX, data = 0x%016llX)", addr, *data64);
}

static inline void sl_core_lgrp_write64(struct sl_core_lgrp *core_lgrp, u64 addr, u64 data64)
{
	core_lgrp->core_ldev->ops.write64(
		core_lgrp->core_ldev->accessors.pci, addr, data64);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"WR64 (addr = 0x%016llX, data = 0x%016llX)", addr, data64);
}

static inline void sl_core_flush64(struct sl_core_link *core_link, u64 addr)
{
	(void)core_link->core_lgrp->core_ldev->ops.read64(
		core_link->core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_link, SL_CORE_HW_IO_LOG_NAME,
		"FLSH (addr = 0x%016llX)", addr);
}

static inline void sl_core_read64(struct sl_core_link *core_link, u64 addr, u64 *data64)
{
	*data64 = core_link->core_lgrp->core_ldev->ops.read64(
		core_link->core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_link, SL_CORE_HW_IO_LOG_NAME,
		"RD64 (addr = 0x%016llX, data = 0x%016llX)", addr, *data64);
}

static inline void sl_core_write64(struct sl_core_link *core_link, u64 addr, u64 data64)
{
	core_link->core_lgrp->core_ldev->ops.write64(
		core_link->core_lgrp->core_ldev->accessors.pci, addr, data64);

	sl_core_log_dbg(core_link, SL_CORE_HW_IO_LOG_NAME,
		"WR64 (addr = 0x%016llX, data = 0x%016llX)", addr, data64);
}

static inline void sl_core_mac_flush64(struct sl_core_mac *core_mac, u64 addr)
{
	(void)core_mac->core_lgrp->core_ldev->ops.read64(
		core_mac->core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_mac, SL_CORE_HW_IO_LOG_NAME,
		"FLSH (addr = 0x%016llX)", addr);
}

static inline void sl_core_mac_read64(struct sl_core_mac *core_mac, u64 addr, u64 *data64)
{
	*data64 = core_mac->core_lgrp->core_ldev->ops.read64(
		core_mac->core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_mac, SL_CORE_HW_IO_LOG_NAME,
		"RD64 (addr = 0x%016llX, data = 0x%016llX)", addr, *data64);
}

static inline void sl_core_mac_write64(struct sl_core_mac *core_mac, u64 addr, u64 data64)
{
	core_mac->core_lgrp->core_ldev->ops.write64(
		core_mac->core_lgrp->core_ldev->accessors.pci, addr, data64);

	sl_core_log_dbg(core_mac, SL_CORE_HW_IO_LOG_NAME,
		"WR64 (addr = 0x%016llX, data = 0x%016llX)", addr, data64);
}

static inline void sl_core_llr_write64(struct sl_core_llr *core_llr, u64 addr, u64 data64)
{
	core_llr->core_lgrp->core_ldev->ops.write64(
		core_llr->core_lgrp->core_ldev->accessors.pci, addr, data64);

	sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME,
		"WR64 (addr = 0x%016llX, data = 0x%016llX)", addr, data64);
}

static inline void sl_core_llr_flush64(struct sl_core_llr *core_llr, u64 addr)
{
	(void)core_llr->core_lgrp->core_ldev->ops.read64(
		core_llr->core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME,
		"FLSH (addr = 0x%016llX)", addr);
}

static inline void sl_core_llr_read64(struct sl_core_llr *core_llr, u64 addr, u64 *data64)
{
	*data64 = core_llr->core_lgrp->core_ldev->ops.read64(
		core_llr->core_lgrp->core_ldev->accessors.pci, addr);

	sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME,
		"RD64 (addr = 0x%016llX, data = 0x%016llX)", addr, *data64);
}

static inline int sl_core_pmi_rd(struct sl_core_lgrp *core_lgrp, u32 addr, u16 *data)
{
	int rtn;

	rtn = core_lgrp->core_ldev->ops.pmi_op(
		core_lgrp->core_ldev->accessors.pmi,
		core_lgrp->num, SL_PMI_OP_RD, addr, data, 0, 0);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"PMI RD (rtn = %d, addr = 0x%08X, data = 0x%04X)", rtn, addr, *data);

	return rtn;
}

static inline int sl_core_pmi_wr(struct sl_core_lgrp *core_lgrp, u32 addr, u16 data, u16 mask)
{
	int rtn;

	rtn = core_lgrp->core_ldev->ops.pmi_op(
		core_lgrp->core_ldev->accessors.pmi,
		core_lgrp->num, SL_PMI_OP_WR, addr, NULL, data, mask);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"PMI WR (rtn = %d, addr = 0x%08X, data = 0x%04X, mask = 0x%04X)",
		rtn, addr, data, mask);

	return rtn;
}

static inline int sl_core_sbus_wr(struct sl_core_lgrp *core_lgrp, u32 dev_addr, u8 reg, u32 data)
{
	int rtn;

	rtn = core_lgrp->core_ldev->ops.sbus_op(
		core_lgrp->core_ldev->accessors.sbus,
		SL_SBUS_OP_WR, core_lgrp->serdes.dt.sbus_ring, dev_addr, reg, NULL, data);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"SBUS WR (rtn = %d, dev_addr = 0x%02X, reg = 0x%02X, data = 0x%08X)",
		rtn, dev_addr, reg, data);

	return rtn;
}

static inline int sl_core_sbus_rd(struct sl_core_lgrp *core_lgrp, u32 dev_addr, u8 reg, u32 *data)
{
	int rtn;

	rtn = core_lgrp->core_ldev->ops.sbus_op(
		core_lgrp->core_ldev->accessors.sbus,
		SL_SBUS_OP_RD, core_lgrp->serdes.dt.sbus_ring, dev_addr, reg, data, 0);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"SBUS RD (rtn = %d, dev_addr = 0x%02X, reg = 0x%02X, data = 0x%08X)",
		rtn, dev_addr, reg, *data);

	return rtn;
}

static inline int sl_core_sbus_rst(struct sl_core_lgrp *core_lgrp, u32 dev_addr)
{
	int rtn;

	rtn = core_lgrp->core_ldev->ops.sbus_op(
		core_lgrp->core_ldev->accessors.sbus,
		SL_SBUS_OP_RST, core_lgrp->serdes.dt.sbus_ring, dev_addr, 0, NULL, 0);

	sl_core_log_dbg(core_lgrp, SL_CORE_HW_IO_LOG_NAME,
		"SBUS RST (rtn = %d, dev_addr = 0x%02X", rtn, dev_addr);

	return rtn;
}

static inline int sl_core_hw_intr_register(struct sl_core_link *link,
	u64 *err_flgs, sl_intr_handler_t handler, void *data)
{
	sl_core_log_dbg(link, SL_CORE_HW_IO_LOG_NAME,
		"REG (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	return link->core_lgrp->core_ldev->ops.pml_intr_register(
		link->core_lgrp->core_ldev->accessors.intr,
		link->core_lgrp->num, "link", err_flgs, handler, data);
}

static inline int sl_core_hw_intr_unregister(struct sl_core_link *link,
	u64 *err_flgs, sl_intr_handler_t handler)
{
	sl_core_log_dbg(link, SL_CORE_HW_IO_LOG_NAME,
		"UNREG (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	return link->core_lgrp->core_ldev->ops.pml_intr_unregister(
		link->core_lgrp->core_ldev->accessors.intr,
		link->core_lgrp->num, err_flgs, handler);
}

static inline int sl_core_hw_intr_enable(struct sl_core_link *core_link,
	u64 *err_flgs, sl_intr_handler_t handler)
{
	sl_core_log_dbg(core_link, SL_CORE_HW_IO_LOG_NAME,
		"EN (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, SL_CORE_HW_IO_LOG_NAME, "canceled");
		return 0;
	}

	return core_link->core_lgrp->core_ldev->ops.pml_intr_enable(
		core_link->core_lgrp->core_ldev->accessors.intr,
		core_link->core_lgrp->num, err_flgs, handler);
}

static inline int sl_core_hw_intr_disable(struct sl_core_link *link,
	u64 *err_flgs, sl_intr_handler_t handler)
{
	sl_core_log_dbg(link, SL_CORE_HW_IO_LOG_NAME,
		"DIS (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	return link->core_lgrp->core_ldev->ops.pml_intr_disable(
		link->core_lgrp->core_ldev->accessors.intr,
		link->core_lgrp->num, err_flgs, handler);
}

static inline int sl_core_hw_intr_llr_register(struct sl_core_llr *core_llr,
	u64 *err_flgs, sl_intr_handler_t handler, void *data)
{
	sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME,
		"REG (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	return core_llr->core_lgrp->core_ldev->ops.pml_intr_register(
		core_llr->core_lgrp->core_ldev->accessors.intr,
		core_llr->core_lgrp->num, "llr", err_flgs, handler, data);
}

static inline int sl_core_hw_intr_llr_unregister(struct sl_core_llr *core_llr,
	u64 *err_flgs, sl_intr_handler_t handler)
{
	sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME,
		"UNREG (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	return core_llr->core_lgrp->core_ldev->ops.pml_intr_unregister(
		core_llr->core_lgrp->core_ldev->accessors.intr,
		core_llr->core_lgrp->num, err_flgs, handler);
}

static inline int sl_core_hw_intr_llr_enable(struct sl_core_llr *core_llr,
	u64 *err_flgs, sl_intr_handler_t handler)
{
	sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME,
		"EN (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	if (sl_core_llr_should_stop(core_llr)) {
		sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME, "canceled");
		return 0;
	}

	return core_llr->core_lgrp->core_ldev->ops.pml_intr_enable(
		core_llr->core_lgrp->core_ldev->accessors.intr,
		core_llr->core_lgrp->num, err_flgs, handler);
}

static inline int sl_core_hw_intr_llr_disable(struct sl_core_llr *core_llr,
	u64 *err_flgs, sl_intr_handler_t handler)
{
	sl_core_log_dbg(core_llr, SL_CORE_HW_IO_LOG_NAME,
		"DIS (hdlr = 0x%p, ptr = 0x%p, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		handler, err_flgs, err_flgs[0], err_flgs[1], err_flgs[2], err_flgs[3]);

	return core_llr->core_lgrp->core_ldev->ops.pml_intr_disable(
		core_llr->core_lgrp->core_ldev->accessors.intr,
		core_llr->core_lgrp->num, err_flgs, handler);
}

#endif /* _SL_CORE_HW_IO_H_ */
