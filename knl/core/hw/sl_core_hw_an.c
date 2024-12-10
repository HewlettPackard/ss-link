// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/random.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_asic.h"
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
#include "hw/sl_core_hw_an.h"
#include "hw/sl_core_hw_pcs.h"

#define LOG_NAME SL_CORE_HW_AN_LOG_NAME

void sl_core_hw_an_tx_pages_encode(struct sl_core_link *core_link, struct sl_link_caps *link_caps)
{
	int x;

	memset(core_link->an.tx_pages, 0, sizeof(core_link->an.tx_pages));

	/* 1 - base page */
	core_link->an.tx_pages[0] |= SL_CORE_HW_AN_BP_802_3;
	core_link->an.tx_pages[0] |= ((u64)(link_caps->pause_map & SL_LINK_CONFIG_PAUSE_MASK) <<
		SL_CORE_HW_AN_BP_PAUSE_SHIFT);
	core_link->an.tx_pages[0] |= ((u64)(link_caps->tech_map & SL_LGRP_CONFIG_TECH_MASK) <<
		SL_CORE_HW_AN_BP_TECH_SHIFT);
	// FIXME: for now just allow the RS bit
	core_link->an.tx_pages[0] |= ((u64)((link_caps->fec_map & SL_LGRP_CONFIG_FEC_MASK) & 0x1) <<
		SL_CORE_HW_AN_BP_FEC_SHIFT);

	/* 2 - OUI message next page */
	set_bit(SL_CORE_HW_AN_NP_BIT_MP, (unsigned long *)&(core_link->an.tx_pages[1]));
	core_link->an.tx_pages[1] |= SL_CORE_HW_AN_NP_MSG_OUI_EXTD;
	core_link->an.tx_pages[1] |= SL_CORE_HW_AN_NP_OUI_HPE;

	/* 3 - unformatted page with OUI message next page */
	// FIXME: send version one until we have a SBL fix
	//core_link->an.tx_pages[2] |= SL_CORE_HW_AN_NP_OUI_VER_0_2;
	core_link->an.tx_pages[2] |= SL_CORE_HW_AN_NP_OUI_VER_0_1;
	core_link->an.tx_pages[2] |= ((link_caps->hpe_map & SL_LINK_CONFIG_HPE_MASK) <<
		SL_CORE_HW_AN_NP_HPE_SHIFT);
	// FIXME: get from hardware?
	core_link->an.tx_pages[2] |= (2ULL << SL_CORE_HW_AN_NP_BIT_OUI_ASIC_VER);

	core_link->an.tx_count = 3;

	sl_core_log_dbg(core_link, LOG_NAME, "tx pages encode (count = %u)", core_link->an.tx_count);
	for (x = 0; x < core_link->an.tx_count; ++x)
		sl_core_log_dbg(core_link, LOG_NAME,
			"tx pages encode pages (%d = 0x%016llX)", x, core_link->an.tx_pages[x]);
}

#define SL_CORE_HW_AN_NUONCE_MASK 0xF
static u64 sl_core_hw_an_nonce_get(struct sl_core_link *core_link)
{
	u8 num;

	do {
		get_random_bytes(&num, sizeof(num));
		num &= SL_CORE_HW_AN_NUONCE_MASK;
	} while (num == 0);

	sl_core_log_dbg(core_link, LOG_NAME, "nonce = 0x%X", num);

	return num;
}

void sl_core_hw_an_stop(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "stop (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_RESET_UPDATE(data64, 1);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_SUBPORT_ENABLE_AUTO_NEG_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num));
}

static void sl_core_hw_an_start(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "start (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_SUBPORT_ENABLE_AUTO_NEG_UPDATE(data64, 1);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_RESET_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num));
}

void sl_core_hw_an_init(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "init");

	core_link->an.state    = SL_CORE_HW_AN_STATE_START;
	core_link->an.page_num = 0;
	core_link->an.tx_count = 0;
	core_link->an.rx_count = 0;
	memset(core_link->an.rx_pages, 0, sizeof(core_link->an.rx_pages));
}

void sl_core_hw_an_config(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;
#ifdef BUILDSYS_FRAMEWORK_CASSINI
	union ss2_port_pml_cfg_pcs_autoneg_timers autoneg_timers = {
		.link_fail_inhibit_timer_max = 0x3FFFFFFFF,
	};
#endif /* BUILDSYS_FRAMEWORK_CASSINI */

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "config (port = %d)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_SUBPORT_PCS_ENABLE_UPDATE(data64, 0);
	data64 = SS2_PORT_PML_CFG_PCS_SUBPORT_ENABLE_AUTO_NEG_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_RESET_UPDATE(data64, 1);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_RESTART_UPDATE(data64, 0);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_NEXT_PAGE_LOADED_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), data64);

	sl_core_hw_pcs_config_swizzles(core_link);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_LOCK_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), data64);

#ifdef BUILDSYS_FRAMEWORK_ROSETTA
	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8, &data64);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS_WORD1_LINK_FAIL_INHIBIT_TIMER_MAX_UPDATE(data64, 0x3FFFFFFFF);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8, data64);
	if (SL_PLATFORM_IS_EMULATOR(core_link->core_lgrp->core_ldev)) {
		sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS, &data64);
		data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS_WORD0_BREAK_LINK_TIMER_MAX_UPDATE(data64, 50000);
		sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS, data64);
	}
#else /* Cassini */
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_TIMERS + 8, autoneg_timers.qw[1]);
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_RX_PCS);
}

static void sl_core_hw_an_base_page_write(struct sl_core_link *core_link)
{
	u32 port;
	u64 page;

	port = core_link->core_lgrp->num;
	page = core_link->an.tx_pages[core_link->an.page_num];

	/* add control bits */
	if (core_link->an.page_num < core_link->an.tx_count - 1)
		set_bit(SL_CORE_HW_AN_BP_BIT_NP, (unsigned long *)&(page));
	page |= (sl_core_hw_an_nonce_get(core_link) << SL_CORE_HW_AN_BP_BIT_TRANS_NONCE);

	/* store toggle bit */
	core_link->an.toggle = test_bit(SL_CORE_HW_AN_NP_BIT_T, (unsigned long *)&(page));

	sl_core_log_dbg(core_link, LOG_NAME,
		"base page write (port = %u, pg %u = 0x%016llX, toggle = %d)",
		port, core_link->an.page_num, page, core_link->an.toggle);

	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_BASE_PAGE(core_link->num), page);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_BASE_PAGE(core_link->num));
}

static void sl_core_hw_an_next_page_write(struct sl_core_link *core_link)
{
	u32 port;
	u64 page;

	port = core_link->core_lgrp->num;
	if (core_link->an.page_num < core_link->an.tx_count)
		page = core_link->an.tx_pages[core_link->an.page_num];
	else
		page = SL_CORE_HW_AN_NP_MSG_NULL;

	/* add control bits */
	if (core_link->an.page_num < core_link->an.tx_count - 1)
		set_bit(SL_CORE_HW_AN_NP_BIT_NP, (unsigned long *)&(page));
	if (core_link->an.toggle)
		set_bit(SL_CORE_HW_AN_NP_BIT_T, (unsigned long *)&(page));
	core_link->an.toggle = !core_link->an.toggle;

	sl_core_log_dbg(core_link, LOG_NAME,
		"next page write (port = %u, pg %u = 0x%016llX)",
		port, core_link->an.page_num, page);

	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_NEXT_PAGE(core_link->num), page);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG_NEXT_PAGE(core_link->num));
}

static void sl_core_hw_an_next_page_go(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "next page go (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_AUTONEG_NEXT_PAGE_LOADED_UPDATE(data64, 1);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num), data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_AUTONEG(core_link->num));
}

int sl_core_hw_an_base_page_send(struct sl_core_link *core_link)
{
	int rtn;

	sl_core_log_dbg(core_link, LOG_NAME, "base page send (pg = %u)", core_link->an.page_num);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_NEXT_PAGE);
	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_ERROR);
	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_DONE);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_AN_BASE_PAGE);
	core_link->an.state = SL_CORE_HW_AN_STATE_BASE;

	sl_core_hw_an_base_page_write(core_link);

	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_AN_PAGE_RECV);

	rtn = sl_core_hw_intr_enable(core_link,
		core_link->intrs[SL_CORE_HW_INTR_AN_PAGE_RECV].flgs, sl_core_hw_an_intr_hdlr);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME,
			"base page send intr enable failed [%d]", rtn);
		return rtn;
	}

	sl_core_hw_an_start(core_link);

	return 0;
}

static int sl_core_hw_an_next_page_send(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "next page send (pg = %u)", core_link->an.page_num);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_BASE_PAGE);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_AN_NEXT_PAGE);
	core_link->an.state = SL_CORE_HW_AN_STATE_NEXT;

	sl_core_hw_an_next_page_write(core_link);

	sl_core_hw_an_next_page_go(core_link);

	return 0;
}

static void sl_core_hw_an_base_page_store(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE(core_link->num), &data64);

	sl_core_log_dbg(core_link, LOG_NAME,
		"base page store (port = %u, pg %u = 0x%016llX)",
		port, core_link->an.page_num, data64);

	if (SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_ABILITY_GET(data64) != 1) {
		sl_core_log_err(core_link, LOG_NAME,
			"base page store lp ability not set (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		return;
	}

	if (SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_BASE_PAGE_GET(data64) != 1) {
		sl_core_log_err(core_link, LOG_NAME,
			"base page store base page not set (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		return;
	}

	core_link->an.rx_pages[core_link->an.page_num] =
		SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_BASE_PAGE_GET(data64);
	core_link->an.rx_count++;

	if (SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_COMPLETE_GET(data64) == 1) {
		core_link->an.state = SL_CORE_HW_AN_STATE_COMPLETE;
		return;
	}

	if (!test_bit(SL_CORE_HW_AN_BP_BIT_NP,
		(unsigned long *)&(core_link->an.rx_pages[core_link->an.page_num]))) {
		core_link->an.state = SL_CORE_HW_AN_STATE_LP_DONE;
		return;
	}
}

static void sl_core_hw_an_next_page_store(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE(core_link->num), &data64);

	sl_core_log_dbg(core_link, LOG_NAME,
		"next page store (port = %u, pg %u = 0x%016llX)",
		port, core_link->an.page_num, data64);

	if (SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_BASE_PAGE_GET(data64) != 0) {
		//FIXME: for now to avoid spam dmesg (need sysfs implementation for this error later)
		sl_core_log_err_trace(core_link, LOG_NAME,
			"next page store base page set (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		core_link->an.state = SL_CORE_HW_AN_STATE_RETRY;
		return;
	}

	core_link->an.rx_pages[core_link->an.page_num] =
		SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_NEXT_PAGE_GET(data64);
	core_link->an.rx_count++;

	if (SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_COMPLETE_GET(data64) == 1) {
		core_link->an.state = SL_CORE_HW_AN_STATE_COMPLETE;
		return;
	}

	if (!test_bit(SL_CORE_HW_AN_NP_BIT_NP,
		(unsigned long *)&(core_link->an.rx_pages[core_link->an.page_num]))) {
		core_link->an.state = SL_CORE_HW_AN_STATE_LP_DONE;
		return;
	}
}

static void sl_core_hw_an_next_page_check(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE(core_link->num), &data64);

	sl_core_log_dbg(core_link, LOG_NAME,
		"next page check (port = %u, data = 0x%016llX)", port, data64);

	// FIXME: any other checks here?

	if (SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_COMPLETE_GET(data64) == 1) {
		core_link->an.state = SL_CORE_HW_AN_STATE_COMPLETE;
		return;
	}
}

void sl_core_hw_an_rx_pages_decode(struct sl_core_link *core_link, struct sl_link_caps *link_caps)
{
	int x;
	u32 pause_map;
	u32 tech_map;
	u32 fec_map;
	u32 hpe_map;
	u64 oui_ver;

	sl_core_log_dbg(core_link, LOG_NAME, "rx pages decode (count = %u)", core_link->an.rx_count);
	for (x = 0; x < core_link->an.tx_count; ++x)
		sl_core_log_dbg(core_link, LOG_NAME,
		"rx pages decode pages (%d = 0x%016llX)", x, core_link->an.rx_pages[x]);

	/* base page deconstruct */
	pause_map = (core_link->an.rx_pages[0] >> SL_CORE_HW_AN_BP_PAUSE_SHIFT);
	pause_map &= SL_LINK_CONFIG_PAUSE_MASK;
	pause_map &= core_link->config.pause_map;
	tech_map  = (core_link->an.rx_pages[0] >> SL_CORE_HW_AN_BP_TECH_SHIFT);
	tech_map  &= SL_LGRP_CONFIG_TECH_MASK;
	tech_map  &= core_link->core_lgrp->config.tech_map;
	fec_map   = (core_link->an.rx_pages[0] >> SL_CORE_HW_AN_BP_FEC_SHIFT);
	fec_map   &= SL_LGRP_CONFIG_FEC_MASK;
	fec_map   &= core_link->core_lgrp->config.fec_map;

	/* next pages deconstruct */
	hpe_map = 0;
	for (x = 1; x < core_link->an.rx_count; ++x) {
		if (!test_bit(SL_CORE_HW_AN_NP_BIT_MP, (unsigned long *)&(core_link->an.rx_pages[x]))) {
			sl_core_log_warn(core_link, LOG_NAME, "rx pages decode no next page");
			continue;
		}
		if ((core_link->an.rx_pages[x] & SL_CORE_HW_AN_NP_MSG_MASK) != SL_CORE_HW_AN_NP_MSG_OUI_EXTD) {
			sl_core_log_warn(core_link, LOG_NAME, "rx pages decode oui extended");
			continue;
		}
		if ((core_link->an.rx_pages[x] & SL_CORE_HW_AN_NP_OUI_HPE_MASK) != SL_CORE_HW_AN_NP_OUI_HPE) {
			sl_core_log_warn(core_link, LOG_NAME, "rx pages decode no oui hpe");
			continue;
		}

		x++; /* look ahead */
		oui_ver = (core_link->an.rx_pages[x] & SL_CORE_HW_AN_NP_OUI_VER_MASK);
		sl_core_log_dbg(core_link, LOG_NAME, "rx pages decode (oui ver = 0x%llX)", oui_ver);
		switch (oui_ver) {
		case SL_CORE_HW_AN_NP_OUI_VER_0_2:
			sl_core_log_dbg(core_link, LOG_NAME, "rx pages decode v2 oui");
			fallthrough;
		case SL_CORE_HW_AN_NP_OUI_VER_0_1:
			sl_core_log_dbg(core_link, LOG_NAME, "rx pages decode v1 oui");
			hpe_map = (core_link->an.rx_pages[x] >> SL_CORE_HW_AN_NP_HPE_SHIFT);
			hpe_map &= SL_LINK_CONFIG_HPE_MASK;
			hpe_map &= core_link->config.hpe_map;
			break;
		}
	}

	/* speed */
	for (x = 31; x >= 0; --x) {
		if (tech_map & BIT(x)) {
			link_caps->tech_map = BIT_ULL(x);
			sl_core_log_dbg(core_link, LOG_NAME,
				"rx pages decode (tech bit = %d)", x);
			break;
		}
	}

	link_caps->pause_map = pause_map;
	link_caps->fec_map   = fec_map;
	link_caps->fec_map  |= SL_LGRP_CONFIG_FEC_RS;
	link_caps->hpe_map   = hpe_map;

	sl_core_log_dbg(core_link, LOG_NAME,
		"rx pages decode (pause = 0x%X, tech = 0x%X, fec = 0x%X, hpe = 0x%X)",
		link_caps->pause_map, link_caps->tech_map, link_caps->fec_map, link_caps->hpe_map);
}

static void sl_core_hw_an_page_recv_intr(struct sl_core_link *core_link)
{
	int                  rtn;

	sl_core_log_dbg(core_link, LOG_NAME,
		"page recv intr (state = %u)", core_link->an.state);

	switch (core_link->an.state) {
	case SL_CORE_HW_AN_STATE_BASE:
		sl_core_hw_an_base_page_store(core_link);
		break;
	case SL_CORE_HW_AN_STATE_NEXT:
		sl_core_hw_an_next_page_store(core_link);
		break;
	case SL_CORE_HW_AN_STATE_LP_DONE:
		sl_core_hw_an_next_page_check(core_link);
		break;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"page recv intr invalid (state = %u)", core_link->an.state);
		goto out;
	}

	if (core_link->an.state == SL_CORE_HW_AN_STATE_COMPLETE) {
		sl_core_log_dbg(core_link, LOG_NAME, "page recv intr autoneg complete");
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_NEXT_PAGE);
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_BASE_PAGE);
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_ERROR);
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_AN_DONE);
		goto out;
	}

	if (core_link->an.state == SL_CORE_HW_AN_STATE_ERROR) {
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_NEXT_PAGE);
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_BASE_PAGE);
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_DONE);
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_AN_ERROR);
		goto out;
	}

	if (core_link->an.state == SL_CORE_HW_AN_STATE_RETRY) {
		rtn = sl_core_hw_an_next_page_send(core_link);
		if (rtn != 0) {
			sl_core_log_err(core_link, LOG_NAME,
				"page recv intr an next page send retry failed [%d]", rtn);
			core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
			goto out;
		}
		return;
	}

	core_link->an.page_num++;

	if (core_link->an.page_num >= SL_CORE_LINK_AN_MAX_PAGES) {
		sl_core_log_err(core_link, LOG_NAME,
			"page recv intr out of pages");
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		goto out;
	}

	rtn = sl_core_hw_an_next_page_send(core_link);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME,
			"page recv intr an next page send failed [%d]", rtn);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		goto out;
	}

	return;

out:
	rtn = sl_core_hw_intr_disable(core_link,
		core_link->intrs[SL_CORE_HW_INTR_AN_PAGE_RECV].flgs, sl_core_hw_an_intr_hdlr);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME, "page recv intr disable failed [%d]", rtn);

	sl_core_work_link_queue(core_link, core_link->an.done_work_num);
}

void sl_core_hw_an_intr_hdlr(u64 *err_flgs, int num_err_flgs, void *data)
{
	struct sl_core_hw_intr_data *info;
	struct sl_core_link         *core_link;

	info = data;
	core_link = info->link;

	sl_core_log_dbg(core_link, LOG_NAME,
		"hdlr %s (link = 0x%p, intr = %u)",
		info->log, core_link, info->intr_num);

	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_AN_PAGE_RECV);

	sl_core_hw_an_page_recv_intr(core_link);
}
