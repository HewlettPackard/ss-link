// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/random.h>
#include <linux/workqueue.h>

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
#include "sl_ctrl_link_counters.h"
#include "sl_ctrl_link.h"

#define LOG_NAME SL_CORE_HW_AN_LOG_NAME

#define SL_CORE_HW_AN_STATE_COMPLETE_ACK  5
#define SL_CORE_HW_AN_STATE_AN_GOOD_CHECK 9

void sl_core_hw_an_tx_pages_encode(struct sl_core_link *core_link, struct sl_link_caps *my_caps)
{
	int x;

	memset(core_link->an.tx_pages, 0, sizeof(core_link->an.tx_pages));

	/* 1 - base page */
	core_link->an.tx_pages[0] |= SL_CORE_HW_AN_BP_802_3;
	core_link->an.tx_pages[0] |= ((u64)(my_caps->pause_map & SL_LINK_CONFIG_PAUSE_MASK) <<
		SL_CORE_HW_AN_BP_PAUSE_SHIFT);
	core_link->an.tx_pages[0] |= ((u64)(my_caps->tech_map & SL_LGRP_CONFIG_TECH_MASK) <<
		SL_CORE_HW_AN_BP_TECH_SHIFT);
	core_link->an.tx_pages[0] |= ((u64)(my_caps->fec_map & SL_LGRP_CONFIG_FEC_MASK) <<
		SL_CORE_HW_AN_BP_FEC_SHIFT);

	/* 2 - OUI message next page */
	set_bit(SL_CORE_HW_AN_NP_BIT_MP, (unsigned long *)&(core_link->an.tx_pages[1]));
	core_link->an.tx_pages[1] |= SL_CORE_HW_AN_NP_MSG_OUI_EXTD;
	core_link->an.tx_pages[1] |= SL_CORE_HW_AN_NP_OUI_HPE;

	/* 3 - unformatted page with OUI message next page */
	// FIXME: send version one until we have a SBL fix
	//core_link->an.tx_pages[2] |= SL_CORE_HW_AN_NP_OUI_VER_0_2;
	core_link->an.tx_pages[2] |= SL_CORE_HW_AN_NP_OUI_VER_0_1;
	core_link->an.tx_pages[2] |= ((my_caps->hpe_map & SL_LINK_CONFIG_HPE_MASK) <<
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

	sl_core_hw_an_config_timers(core_link);

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
		"next page write (port = %u, pg %u = 0x%016llX, toggle = %d)",
		port, core_link->an.page_num, page, core_link->an.toggle);

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
		sl_core_log_err_trace(core_link, LOG_NAME,
			"base page send intr enable failed [%d]", rtn);
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_BP_SEND_INTR_ENABLE_FAIL);
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
	u64 state;

	port = core_link->core_lgrp->num;

	sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE(core_link->num), &data64);

	sl_core_log_dbg(core_link, LOG_NAME,
		"base page store (port = %u, pg %u = 0x%016llX, state = %llu)",
		port, core_link->an.page_num, data64,
		SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(data64));

	state = SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(data64);
	if ((state != SL_CORE_HW_AN_STATE_COMPLETE_ACK) &&
		(state != SL_CORE_HW_AN_STATE_AN_GOOD_CHECK)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"base page store bad state (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		sl_core_data_link_an_fail_cause_set(core_link, SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_STATE_BAD);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		return;
	}

	if (!SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_ABILITY_GET(data64)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"base page store lp ability not set (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_LP_ABILITY_NOT_SET);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		return;
	}

	if (!SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_BASE_PAGE_GET(data64)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"base page store base page not set (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_BP_NOT_SET);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		return;
	}

	core_link->an.rx_pages[core_link->an.page_num] =
		SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_BASE_PAGE_GET(data64);
	core_link->an.rx_count++;

	if (SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_COMPLETE_GET(data64)) {
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
	u64 state;

	port = core_link->core_lgrp->num;

	sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE(core_link->num), &data64);

	sl_core_log_dbg(core_link, LOG_NAME,
		"next page store (port = %u, pg %u = 0x%016llX, state = %llu)",
		port, core_link->an.page_num, data64,
		SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(data64));

	state = SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(data64);
	if ((state != SL_CORE_HW_AN_STATE_COMPLETE_ACK) &&
		(state != SL_CORE_HW_AN_STATE_AN_GOOD_CHECK)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"next page store bad state (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		sl_core_data_link_an_fail_cause_set(core_link, SL_CORE_HW_AN_FAIL_CAUSE_NP_STORE_STATE_BAD);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		return;
	}

	if (SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_BASE_PAGE_GET(data64)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"next page store base page set (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_NP_STORE_BP_SET);
		core_link->an.state = SL_CORE_HW_AN_STATE_RETRY;
		return;
	}

	core_link->an.rx_pages[core_link->an.page_num] =
		SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_NEXT_PAGE_GET(data64);
	core_link->an.rx_count++;

	if (SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_COMPLETE_GET(data64)) {
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
	u64 state;

	port = core_link->core_lgrp->num;

	sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE(core_link->num), &data64);

	sl_core_log_dbg(core_link, LOG_NAME,
		"next page check (port = %u, data = 0x%016llX)", port, data64);

	state = SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(data64);
	if ((state != SL_CORE_HW_AN_STATE_COMPLETE_ACK) &&
		(state != SL_CORE_HW_AN_STATE_AN_GOOD_CHECK)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"next page check bad state (pg %u = 0x%016llX)",
			core_link->an.page_num, data64);
		sl_core_data_link_an_fail_cause_set(core_link, SL_CORE_HW_AN_FAIL_CAUSE_NP_CHECK_STATE_BAD);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		return;
	}

	if (SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_COMPLETE_GET(data64)) {
		core_link->an.state = SL_CORE_HW_AN_STATE_COMPLETE;
		return;
	}
}

#define AN_IS_OUI_HPE_EXTD(_page)                                                   \
	(((_page) & (SL_CORE_HW_AN_NP_MSG_MASK | SL_CORE_HW_AN_NP_OUI_HPE_MASK)) == \
		(SL_CORE_HW_AN_NP_MSG_OUI_EXTD | SL_CORE_HW_AN_NP_OUI_HPE))
#define AN_OUI_VER_GET(_page) \
	((_page) & SL_CORE_HW_AN_NP_OUI_VER_MASK)
#define AN_HPE_MAP_GET(_page) \
	(((_page) >> SL_CORE_HW_AN_NP_HPE_SHIFT) & SL_LINK_CONFIG_HPE_MASK)

int sl_core_hw_an_rx_pages_decode(struct sl_core_link *core_link,
				  struct sl_link_caps *my_caps, struct sl_link_caps *link_caps)
{
	int x;
	u32 pause_map;
	u32 tech_map;
	u32 fec_map;
	u32 hpe_map;

	sl_core_log_dbg(core_link, LOG_NAME, "rx pages decode (count = %u)", core_link->an.rx_count);
	for (x = 0; x < core_link->an.rx_count; ++x)
		sl_core_log_dbg(core_link, LOG_NAME,
				"rx pages decode pages (%d = 0x%016llX)", x, core_link->an.rx_pages[x]);

	/* base page */
	if (!(core_link->an.rx_pages[0] & SL_CORE_HW_AN_BP_802_3)) {
		sl_core_log_err_trace(core_link, LOG_NAME, "rx pages decode no base page");
		sl_core_data_link_an_fail_cause_set(core_link,
						    SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_NO_BP);
		return -EIO;
	}
	pause_map  = (core_link->an.rx_pages[0] >> SL_CORE_HW_AN_BP_PAUSE_SHIFT);
	pause_map &= SL_LINK_CONFIG_PAUSE_MASK;
	pause_map &= core_link->config.pause_map;
	tech_map   = (core_link->an.rx_pages[0] >> SL_CORE_HW_AN_BP_TECH_SHIFT);
	tech_map  &= SL_LGRP_CONFIG_TECH_MASK;
	tech_map  &= core_link->core_lgrp->config.tech_map;
	fec_map    = (core_link->an.rx_pages[0] >> SL_CORE_HW_AN_BP_FEC_SHIFT);
	fec_map   &= SL_LGRP_CONFIG_FEC_MASK;
	fec_map   &= core_link->core_lgrp->config.fec_map;

	/* next pages */
	hpe_map = 0;
	for (x = 1; x < core_link->an.rx_count; ++x) {
		if (AN_IS_OUI_HPE_EXTD(core_link->an.rx_pages[x])) {
			x++;
			switch (AN_OUI_VER_GET(core_link->an.rx_pages[x])) {
			case SL_CORE_HW_AN_NP_OUI_VER_0_2:
				sl_core_log_dbg(core_link, LOG_NAME, "rx pages decode v2 oui");
				fallthrough;
			case SL_CORE_HW_AN_NP_OUI_VER_0_1:
				sl_core_log_dbg(core_link, LOG_NAME, "rx pages decode v1 oui");
				hpe_map = AN_HPE_MAP_GET(core_link->an.rx_pages[x]);
				break;
			default:
				sl_core_log_err_trace(core_link, LOG_NAME,
						      "rx pages decode invalid oui (ver = 0x%llX)",
						      AN_OUI_VER_GET(core_link->an.rx_pages[x]));
				sl_core_data_link_an_fail_cause_set(core_link,
								    SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_OUI_INVALID);
				return -EIO;
			}
			break;
		}
	}

	/* common maps */
	tech_map &= my_caps->tech_map;
	for (x = 31; x >= 0; --x) {
		if (tech_map & BIT(x)) {
			link_caps->tech_map = BIT(x);
			break;
		}
	}
	link_caps->pause_map = pause_map & my_caps->pause_map;
	link_caps->fec_map   = fec_map & my_caps->fec_map;
	//FIXME: SSHOTPLAT-6075 Workaround to support v1 switches.
	link_caps->fec_map  |= SL_LGRP_CONFIG_FEC_RS;
	link_caps->hpe_map   = hpe_map & my_caps->hpe_map;

	sl_core_log_dbg(core_link, LOG_NAME,
			"rx pages decode (pause = 0x%X, tech = 0x%X, fec = 0x%X, hpe = 0x%X)",
			link_caps->pause_map, link_caps->tech_map, link_caps->fec_map, link_caps->hpe_map);

	return 0;
}


/**
 * sl_core_hw_an_page_recv_intr - Handle page AN receive interrupt
 * @core_link: Pointer to the core link structure
 *
 * Context: Interrupt Context
 */
static void sl_core_hw_an_page_recv_intr(struct sl_core_link *core_link)
{
	int rtn;

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
		sl_core_log_err_trace(core_link, LOG_NAME,
			"page recv intr invalid (state = %u)", core_link->an.state);
		sl_core_data_link_an_fail_cause_set(core_link, SL_CORE_HW_AN_FAIL_CAUSE_INTR_STATE_INVALID);
		goto out;
	}

	if (core_link->an.state == SL_CORE_HW_AN_STATE_COMPLETE) {
		sl_core_log_dbg(core_link, LOG_NAME, "page recv intr autoneg complete");
		sl_core_data_link_an_fail_cause_set(core_link, SL_CORE_HW_AN_FAIL_CAUSE_NONE);
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
		atomic_inc(&core_link->an.retry_count);
		rtn = sl_core_hw_an_next_page_send(core_link);
		if (rtn != 0) {
			sl_core_log_err_trace(core_link, LOG_NAME,
				"page recv intr an next page send retry failed [%d]", rtn);
			sl_core_data_link_an_fail_cause_set(core_link,
				SL_CORE_HW_AN_FAIL_CAUSE_INTR_AN_RETRY_NP_SEND_FAIL);
			core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
			goto out;
		}
		return;
	}

	core_link->an.page_num++;

	if (core_link->an.page_num >= SL_CORE_LINK_AN_MAX_PAGES) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"page recv intr out of pages");
		sl_core_data_link_an_fail_cause_set(core_link, SL_CORE_HW_AN_FAIL_CAUSE_INTR_OUT_OF_PAGES);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		goto out;
	}

	rtn = sl_core_hw_an_next_page_send(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"page recv intr an next page send failed [%d]", rtn);
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_INTR_NP_SEND_FAIL);
		core_link->an.state = SL_CORE_HW_AN_STATE_ERROR;
		goto out;
	}

	return;

out:
	rtn = sl_core_hw_intr_disable(core_link,
		core_link->intrs[SL_CORE_HW_INTR_AN_PAGE_RECV].flgs, sl_core_hw_an_intr_hdlr);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME, "page recv intr disable failed [%d]", rtn);

	queue_work(core_link->core_lgrp->core_ldev->workqueue, &(core_link->work[core_link->an.done_work_num]));
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
