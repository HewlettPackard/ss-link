// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/delay.h>

#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_pcs.h"
#include "hw/sl_core_hw_uc_ram.h"
#include "hw/sl_core_hw_settings.h"
#include "data/sl_core_data_lgrp.h"
#include "hw/sl_core_hw_serdes_addrs.h"
#include "data/sl_core_data_link.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

u8 sl_core_hw_serdes_rx_asic_lane_num_get(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = core_link->core_lgrp;

	for (asic_lane_num = 0; asic_lane_num < SL_MAX_LANES; ++asic_lane_num)
		if (core_lgrp->serdes.dt.lane_info[asic_lane_num].rx_source +
				(4 * (core_lgrp->num & BIT(0))) == serdes_lane_num) {
			sl_core_log_dbg(core_lgrp, LOG_NAME,
				"RX asic lane get (asic_lane_num = %u, serdes_lane_num = %u)",
				asic_lane_num, serdes_lane_num);
			return asic_lane_num;
		}

	sl_core_log_err_trace(core_lgrp, LOG_NAME,
		"RX asic lane get not found (serdes_lane_num = %u)", serdes_lane_num);
	return 0;
}

u8 sl_core_hw_serdes_tx_asic_lane_num_get(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = core_link->core_lgrp;

	for (asic_lane_num = 0; asic_lane_num < SL_MAX_LANES; ++asic_lane_num)
		if (core_lgrp->serdes.dt.lane_info[asic_lane_num].tx_source +
				(4 * (core_lgrp->num & BIT(0))) == serdes_lane_num) {
			sl_core_log_dbg(core_lgrp, LOG_NAME,
				"TX asic lane get (asic_lane_num = %u, serdes_lane_num = %u)",
				asic_lane_num, serdes_lane_num);
			return asic_lane_num;
		}

	sl_core_log_err_trace(core_lgrp, LOG_NAME,
		"TX asic lane get not found (serdes_lane_num = %u)", serdes_lane_num);
	return 0;
}

static u8 sl_core_hw_serdes_rx_serdes_lane_num_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num)
{
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"RX serdes lane get (asic_lane_num = %u, serdes_lane_num = %lu)", asic_lane_num,
		core_lgrp->serdes.dt.lane_info[asic_lane_num].rx_source + (4 * (core_lgrp->num & BIT(0))));

	return core_lgrp->serdes.dt.lane_info[asic_lane_num].rx_source + (4 * (core_lgrp->num & BIT(0)));
}

static u8 sl_core_hw_serdes_tx_serdes_lane_num_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num)
{
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"TX serdes lane get (asic_lane_num = %u, serdes_lane_num = %lu)", asic_lane_num,
		core_lgrp->serdes.dt.lane_info[asic_lane_num].tx_source + (4 * (core_lgrp->num & BIT(0))));

	return core_lgrp->serdes.dt.lane_info[asic_lane_num].tx_source + (4 * (core_lgrp->num & BIT(0)));
}

int sl_core_hw_serdes_lane_pre1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre1)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_TX_FED_TXFIR_TAP_CONTROL2], &data16);

	*pre1 = (s16)((data16 & 0x01FF) << 7) >> 7;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"pre1 get (data16 = 0x%X, pre1 = %d)", data16, *pre1);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_pre2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre2)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_TX_FED_TXFIR_TAP_CONTROL1], &data16);

	*pre2 = (s16)((data16 & 0x01FF) << 7) >> 7;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"pre2 get (data16 = 0x%X, pre2 = %d)", data16, *pre2);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_pre3_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre3)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_TX_FED_TXFIR_TAP_CONTROL0], &data16);

	*pre3 = (s16)((data16 & 0x01FF) << 7) >> 7;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"pre3 get (data16 = 0x%X, pre3 = %d)", data16, *pre3);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_cursor_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *cursor)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_TX_FED_TXFIR_TAP_CONTROL3], &data16);

	*cursor = (s16)((data16 & 0x01FF) << 7) >> 7;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"cursor get (data16 = 0x%X, cursor = %d)", data16, *cursor);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_post1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post1)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_TX_FED_TXFIR_TAP_CONTROL4], &data16);

	*post1 = (s16)((data16 & 0x01FF) << 7) >> 7;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"post1 get (data16 = 0x%X, post1 = %d)", data16, *post1);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_post2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post2)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_TX_FED_TXFIR_TAP_CONTROL5], &data16);

	*post2 = (s16)((data16 & 0x01FF) << 7) >> 7;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"post2 get (data16 = 0x%X, post2 = %d)", data16, *post2);

	return 0;
out:
	return rtn;
}

u16 sl_core_hw_serdes_mode(struct sl_core_lgrp *core_lgrp,
	struct sl_core_serdes_settings *settings)
{
	u16 mode;

	mode = settings->osr;

	if (settings->encoding != SL_CORE_HW_SERDES_ENCODING_NRZ)
		mode |= (0x1 << 6);

	mode |= (settings->width << 7);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "serdes mode = 0x%X", mode);

	return mode;
}

int sl_core_hw_serdes_lane_osr_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *osr)
{
	u64 data64;
	u32 port;

	port = core_lgrp->num;

	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);

	*osr = (SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_GET(data64) & 0x3F);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "osr get (port = %u, osr = %u)", port, *osr);

	return 0;
}

int sl_core_hw_serdes_lane_encoding_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *encoding)
{
	int rtn;
	u64 data64;
	u32 port;
	u16 data16;
	u8  serdes_lane_num;

	port = core_lgrp->num;

	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);

	if (SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_GET(data64) & 0x40) {
		serdes_lane_num = sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
			core_lgrp->core_ldev->serdes.addrs[SERDES_CDR_CONTROL_0], &data16); /* rx_pam4_er_mode */
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"encoding get (serdes_lane_num = %u, encoding = 0x%X)",
			serdes_lane_num, (data16 & 0x8000));
		if (data16 & 0x8000)
			*encoding = SL_CORE_HW_SERDES_ENCODING_PAM4_ER;
		else
			*encoding = SL_CORE_HW_SERDES_ENCODING_PAM4_NR;
	} else {
		*encoding = SL_CORE_HW_SERDES_ENCODING_NRZ;
	}

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"encoding get (port = %u, encoding = %u)", port, *encoding);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_width_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *width)
{
	u64 data64;
	u32 port;

	port = core_lgrp->num;

	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);

	*width = ((SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_GET(data64) >> 7) & 0x3);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"width get (port = %u, width = %u)", port, *width);

	return 0;
}

u16 sl_core_hw_serdes_config(struct sl_core_lgrp *core_lgrp,
	struct sl_core_serdes_settings *core_serdes_settings,
	struct sl_media_serdes_settings *media_serdes_settings)
{
	u16 config;

	config = 0;

	config |= ((core_serdes_settings->dfe & 0x1) << 2);

	config |= ((media_serdes_settings->media & 0x3) << 5);

	config |= ((core_serdes_settings->scramble_dis & 0x1) << 8);

	switch (core_serdes_settings->encoding) {
	case SL_CORE_HW_SERDES_ENCODING_PAM4_ER:
		config |= (1 << 11);
		break;
	case SL_CORE_HW_SERDES_ENCODING_PAM4_NR:
		config |= (1 << 14);
		break;
	case SL_CORE_HW_SERDES_ENCODING_NRZ:
		config |= (1 << 15);
		break;
	}

	sl_core_log_dbg(core_lgrp, LOG_NAME, "serdes config = 0x%X", config);

	return config;
}

int sl_core_hw_serdes_lane_dfe_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *dfe)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_rx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_FW_API_DATA0], &data16);

	*dfe = ((data16 >> 2) & 0x1);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_scramble_dis_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *scramble_dis)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = sl_core_hw_serdes_rx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_FW_API_DATA0], &data16);

	*scramble_dis = ((data16 >> 8) & 0x1);

	return 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_lane_up_clock_align(struct sl_core_lgrp *core_lgrp, u8 serdes_lane_num)
{
	int  rtn;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up clock align (serdes_lane_num = %u)", serdes_lane_num);

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_AMS_TX_TXCONTROL_3], 0x0000, 0x0080); /* ams_tx_sel_txmaster */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_5], 0x0000, 0x0800); /* tx_pi_pd_bypass_vco */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_5], 0x0000, 0x0400); /* tx_pi_pd_bypass_flt */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_5], 0x0000, 0x0010); /* tx_pi_hs_fifo_phserr_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_5], 0x0000, 0x0008); /* tx_pi_ext_pd_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_0], 0x0000, 0x0001); /* tx_pi_en */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_0], 0x0000, 0x0002); /* tx_pi_jitter_filter_en */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_0], 0x0000, 0x0004); /* tx_pi_ext_ctrl_en */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_0], 0x0000, 0x7000); /* tx_pi_ext_phase_bwsel_integ */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		addrs[SERDES_TX_PI_TXPICONTROL_5], 0x0000, 0x0800); /* tx_pi_pd_bypass_vco */

	rtn = 0;
out:
	return rtn;
}

#if 0 // FIXME: removed for now, but leave here just in case.
#define LANE_UP_LINKTRAIN_NUM_TRIES      10
#define LANE_UP_LINKTRAIN_FRAME_LOCK_BIT BIT(1)
#define LANE_UP_LINKTRAIN_COMPLETED_BIT  BIT(0)
#define LANE_UP_LINKTRAIN_CHECK_BITS     (LANE_UP_LINKTRAIN_FRAME_LOCK_BIT | \
					  LANE_UP_LINKTRAIN_COMPLETED_BIT)
static int sl_core_hw_serdes_lane_up_linktrain_check(struct sl_core_lgrp *core_lgrp, u8 lane_num)
{
	int rtn;
	u8  x;
	u16 data16;

	for (x = 0; x < LANE_UP_LINKTRAIN_NUM_TRIES; ++x) {
		msleep(200);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0,
			core_lgrp->core_ldev->serdes.addrs[SERDES_LINKTRN_IEEE_TX_LINKTRNIT_BASE_R_PMD_STATUS,
			&data16);
		if ((data16 & LANE_UP_LINKTRAIN_CHECK_BITS) == LANE_UP_LINKTRAIN_CHECK_BITS)
			return 0;
	}

	sl_core_log_err(core_lgrp, LOG_NAME,
		"lane up linktrain check fail (lane = %u, status = 0x%04X)", lane_num, data16);
	rtn = -EIO;

out:
	return rtn;
}
#endif

#define LANE_UP_TX_CHECK_NUM_TRIES 10
#define LANE_UP_TX_DATA_VALID_BIT  BIT(20)
#define LANE_UP_TX_CLOCK_VALID_BIT BIT(16)
#define LANE_UP_TX_CHECK_BITS      (LANE_UP_TX_DATA_VALID_BIT | \
				    LANE_UP_TX_CLOCK_VALID_BIT)
static int sl_core_hw_serdes_lane_up_tx_check(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int                  rtn;
	bool                 is_canceled_or_timed_out;
	u8                   x;
	u32                  port;
	u64                  status;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp     = core_link->core_lgrp;
	port          = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up tx check (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	sl_core_hw_serdes_tx_lane_state_set(core_lgrp,
		asic_lane_num, SL_CORE_HW_SERDES_LANE_STATE_CHECK);

	for (x = 0; x < LANE_UP_TX_CHECK_NUM_TRIES; ++x) {
		rtn = sl_core_link_is_canceled_or_timed_out(core_link, &is_canceled_or_timed_out);
		if (rtn) {
			sl_core_log_err_trace(core_lgrp, LOG_NAME,
					      "lane up tx check - is_canceled_or_timed_out failed [%d]", rtn);
			return rtn;
		}

		if (is_canceled_or_timed_out) {
			sl_core_log_dbg(core_link, LOG_NAME, "lane up tx check canceled");
			return -ECANCELED;
		}
		msleep(100);
		sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_STS_SERDES(asic_lane_num), &status);
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up tx check (asic_lane_num = %u, status = 0x%llX)", asic_lane_num, status);
		if ((status & LANE_UP_TX_CHECK_BITS) == LANE_UP_TX_CHECK_BITS)
			return 0;
	}

	sl_core_log_err_trace(core_lgrp, LOG_NAME,
		"lane up tx check fail (lane = %u, status = 0x%llX)", asic_lane_num, status);
	return -EIO;
}

#define LANE_UP_RX_CHECK_NUM_TRIES        70
#define LANE_UP_RX_CHECK_DATA_VALID_BIT   BIT(12)
#define LANE_UP_RX_CHECK_CLOCK_VALID_BIT  BIT(8)
#define LANE_UP_RX_CHECK_LOCK_BIT         BIT(4)
#define LANE_UP_RX_CHECK_BITS             (LANE_UP_RX_CHECK_DATA_VALID_BIT  | \
					   LANE_UP_RX_CHECK_CLOCK_VALID_BIT | \
					   LANE_UP_RX_CHECK_LOCK_BIT)
/* wait for 6s for PAM4 and 1.5s for NRZ */
static int sl_core_hw_serdes_lane_up_rx_check(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int                  rtn;
	bool                 is_canceled_or_timed_out;
	u8                   x;
	u32                  port;
	u64                  status;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp     = core_link->core_lgrp;
	port          = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up rx check (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	sl_core_hw_serdes_rx_lane_state_set(core_lgrp,
		asic_lane_num, SL_CORE_HW_SERDES_LANE_STATE_CHECK);

	for (x = 0; x < LANE_UP_RX_CHECK_NUM_TRIES; ++x) {
		rtn = sl_core_link_is_canceled_or_timed_out(core_link, &is_canceled_or_timed_out);
		if (rtn) {
			sl_core_log_err_trace(core_lgrp, LOG_NAME,
					      "lane up rx check - is_canceled_or_timed_out failed [%d]", rtn);
			return rtn;
		}

		if (is_canceled_or_timed_out) {
			sl_core_log_dbg(core_link, LOG_NAME, "lane up rx check canceled");
			return -ECANCELED;
		}
		msleep(300);
		sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_STS_SERDES(asic_lane_num), &status);
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up rx check (asic_lane_num = %u, status = 0x%llX)", asic_lane_num, status);
		if ((status & LANE_UP_RX_CHECK_BITS) == LANE_UP_RX_CHECK_BITS)
			return 0;
	}

	sl_core_log_err_trace(core_lgrp, LOG_NAME,
		"lane up rx check fail (asic_lane_num = %u, status = 0x%llX)", asic_lane_num, status);
	return -EIO;
}

#define LANE_UP_QUALITY_CHECK_NUM_TRIES 20
static int sl_core_hw_serdes_lane_up_quality_check(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int                  rtn;
	u8                   x;
	u32                  port;
	u8                   asic_lane_num;
	u32                  addr;
	u8                   eye_upper;
	u8                   eye_lower;
	u16                  data16;
	struct sl_core_lgrp *core_lgrp;
	u16                  extended;
	u64                  data64;
	bool                 is_canceled_or_timed_out;

	core_lgrp     = core_link->core_lgrp;
	port          = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up quality check (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* if link training decides extended reach, then modify eye limits */
	if (is_flag_set(core_link->core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN)) {
		sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
		if (SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_GET(data64) & 0x40) {
			SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
				core_lgrp->core_ldev->serdes.addrs[SERDES_CDR_CONTROL_0], &extended);
			sl_core_log_dbg(core_lgrp, LOG_NAME,
				"extended get (serdes_lane_num = %u, extended = 0x%X)",
				serdes_lane_num, (extended & 0x8000));
			if (extended & 0x8000) {
				sl_core_log_dbg(core_lgrp, LOG_NAME, "lane up quality check extended reach");
				core_link->core_lgrp->serdes.eye_limits[asic_lane_num].low  = 5;
				core_link->core_lgrp->serdes.eye_limits[asic_lane_num].high = 30;
				if (core_link->pcs.settings.pcs_mode == SL_CORE_HW_PCS_MODE_BJ_100G) {
					core_link->core_lgrp->serdes.eye_limits[asic_lane_num].low  = 25;
					core_link->core_lgrp->serdes.eye_limits[asic_lane_num].high = 150;
				}
			}
		}
	}

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x7, serdes_lane_num, core_lgrp);

	for (x = 0; x < LANE_UP_QUALITY_CHECK_NUM_TRIES; ++x) {
		rtn = sl_core_link_is_canceled_or_timed_out(core_link, &is_canceled_or_timed_out);
		if (rtn) {
			sl_core_log_err_trace(core_lgrp, LOG_NAME,
					      "lane up quality check - is_canceled_or_timed_out failed [%d]", rtn);
			return rtn;
		}

		if (is_canceled_or_timed_out) {
			sl_core_log_dbg(core_lgrp, LOG_NAME, "lane up quality check canceled");
			return -ECANCELED;
		}
		msleep(200);
		SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id,
			serdes_lane_num, addr, &eye_upper);
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up quality check (serdes_lane_num = %u, asic_lane_num = %u, eye_upper = %u, low = %u, high = %u)",
			serdes_lane_num, asic_lane_num, eye_upper,
			core_lgrp->serdes.eye_limits[asic_lane_num].low,
			core_lgrp->serdes.eye_limits[asic_lane_num].high);
		if ((eye_upper > core_lgrp->serdes.eye_limits[asic_lane_num].low) &&
			(eye_upper < core_lgrp->serdes.eye_limits[asic_lane_num].high))
			break;
	}
	if (x >= LANE_UP_QUALITY_CHECK_NUM_TRIES)
		return -EIO;

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x8, serdes_lane_num, core_lgrp);

	for (x = 0; x < LANE_UP_QUALITY_CHECK_NUM_TRIES; ++x) {
		rtn = sl_core_link_is_canceled_or_timed_out(core_link, &is_canceled_or_timed_out);
		if (rtn) {
			sl_core_log_err_trace(core_lgrp, LOG_NAME,
					      "lane up quality check - is_canceled_or_timed_out failed [%d]", rtn);
			return rtn;
		}

		if (is_canceled_or_timed_out) {
			sl_core_log_dbg(core_lgrp, LOG_NAME, "lane up quality check canceled");
			return -ECANCELED;
		}
		msleep(200);
		SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id,
			serdes_lane_num, addr, &eye_lower);
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up quality check (serdes_lane_num = %u, asic_lane_num = %u, eye_lower = %u, low = %u, high = %u)",
			serdes_lane_num, asic_lane_num, eye_lower,
			core_lgrp->serdes.eye_limits[asic_lane_num].low,
			core_lgrp->serdes.eye_limits[asic_lane_num].high);
		if ((eye_lower > core_lgrp->serdes.eye_limits[asic_lane_num].low) &&
			(eye_lower < core_lgrp->serdes.eye_limits[asic_lane_num].high))
			break;
	}
	if (x >= LANE_UP_QUALITY_CHECK_NUM_TRIES)
		return -EIO;

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RX_MISC_CONFIG],  &data16);
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up quality check (serdes_lane_num = %u, 0x%X = 0x%X)", serdes_lane_num,
		 core_lgrp->core_ldev->serdes.addrs[SERDES_RX_MISC_CONFIG], data16);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_TLB_TX_TLB_TX_PAM4_CONFIG_0],  &data16);
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up quality check (serdes_lane_num = %u, 0x%X = 0x%X)",
		core_lgrp->core_ldev->serdes.addrs[SERDES_TLB_TX_TLB_TX_PAM4_CONFIG_0],
		serdes_lane_num, data16);

	return 0;
out:
	return rtn;
}

static void sl_core_hw_pmd_tx_enable(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	u64 data64;
	u32 port;
	u8  asic_lane_num;

	port          = core_link->core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_link, LOG_NAME,
		"pmd_tx_enable (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	sl_core_lgrp_read64(core_link->core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_DISABLE_UPDATE(data64, 0);
	sl_core_lgrp_write64(core_link->core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_link->core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));
}

int sl_core_hw_serdes_lanes_up(struct sl_core_link *core_link, bool is_autoneg)
{
	int           rtn;
	unsigned long lane_map;
	u8            serdes_lane_num;

	lane_map = core_link->serdes.lane_map;

	sl_core_log_dbg(core_link, LOG_NAME,
		"lanes up (lane_map = 0x%02lX, is_autoneg = %d)", lane_map, is_autoneg);

	/* start */
	for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
		sl_core_log_dbg(core_link, LOG_NAME, "lanes up (serdes_lane_num = %u)", serdes_lane_num);
		rtn = sl_core_hw_serdes_lane_up_rx_setup(core_link, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_setup failed [%d]", rtn);
			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_tx_setup(core_link, serdes_lane_num, is_autoneg);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_setup failed [%d]", rtn);
			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_rx_config(core_link, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_config failed [%d]", rtn);
			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_tx_config(core_link, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_config failed [%d]", rtn);
			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_rx_start(core_link, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_start failed [%d]", rtn);
			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_tx_start(core_link, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_start failed [%d]", rtn);
			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP);
			goto out;
		}
	}

	/* clock align */
	for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
		rtn = sl_core_hw_serdes_lane_up_clock_align(core_link->core_lgrp, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_clock_align failed [%d]", rtn);
			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP);
			goto out;
		}
	}

#if 0 // FIXME: removed for now, but leave here just in case.
	/* link training check */
	if (is_flag_set(flags, SL_CORE_LINK_CFG_FLAG_LINKTRAIN_ON)) {
		for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
			rtn = sl_core_hw_serdes_lane_up_linktrain_check(core_link->core_lgrp, serdes_lane_num);
			if (rtn) {
				sl_core_log_err(core_link, LOG_NAME,
					"lane_up_linktrain_check failed [%d]", rtn);
				goto out;
			}
		}
	}
#endif

	if (!is_autoneg) {

		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_SERDES_CHECK);

		for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
			rtn = sl_core_hw_serdes_lane_up_tx_check(core_link, serdes_lane_num);
			if (rtn) {
				sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_check failed [%d]", rtn);
				sl_core_data_link_last_up_fail_cause_map_set(core_link,
					SL_LINK_DOWN_CAUSE_SERDES_SIGNAL_MAP);
				goto out;
			}
		}
	}

	/* sleep to give last lane time to settle */
	msleep(20);

	if (!is_autoneg) {
		sl_core_hw_pcs_tx_start(core_link);

		for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES)
			sl_core_hw_pmd_tx_enable(core_link, serdes_lane_num);

		for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
			rtn = sl_core_hw_serdes_lane_up_rx_check(core_link, serdes_lane_num);
			if (rtn) {
				sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_check failed [%d]", rtn);
				sl_core_data_link_last_up_fail_cause_map_set(core_link,
					SL_LINK_DOWN_CAUSE_SERDES_SIGNAL_MAP);
				goto out;
			}
		}

		if (!is_flag_set(core_link->core_lgrp->config.options, SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE)) {
			sl_core_log_dbg(core_link, LOG_NAME, "loopback off so check quality");
			for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
				rtn = sl_core_hw_serdes_lane_up_quality_check(core_link, serdes_lane_num);
				if (rtn) {
					sl_core_log_err_trace(core_link, LOG_NAME,
						"lane_up_quality_check failed [%d]", rtn);
					sl_core_data_link_last_up_fail_cause_map_set(core_link,
						SL_LINK_DOWN_CAUSE_SERDES_QUALITY_MAP);
					goto out;
				}
			}
		}

		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_SERDES_CHECK);
	}

	/* up */
	for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
		sl_core_hw_serdes_tx_lane_state_set(core_link->core_lgrp,
			sl_core_hw_serdes_tx_asic_lane_num_get(core_link, serdes_lane_num),
			SL_CORE_HW_SERDES_LANE_STATE_UP);
		sl_core_hw_serdes_rx_lane_state_set(core_link->core_lgrp,
			sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num),
			SL_CORE_HW_SERDES_LANE_STATE_UP);
	}

	rtn = 0;
out:
	return rtn;
}

void sl_core_hw_serdes_lanes_down(struct sl_core_link *core_link)
{
	unsigned long lane_map;
	u8            serdes_lane_num;

	lane_map = core_link->serdes.lane_map;

	sl_core_log_dbg(core_link, LOG_NAME, "lanes down (lane_map = 0x%02lX)", lane_map);

	for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
		sl_core_hw_serdes_lane_down_tx_stop(core_link, serdes_lane_num);
		sl_core_hw_serdes_lane_down_rx_stop(core_link, serdes_lane_num);
	}

	for_each_set_bit(serdes_lane_num, &lane_map, SL_MAX_SERDES_LANES) {
		sl_core_hw_serdes_tx_lane_state_set(core_link->core_lgrp,
			sl_core_hw_serdes_tx_asic_lane_num_get(core_link, serdes_lane_num),
			SL_CORE_HW_SERDES_LANE_STATE_DOWN);
		sl_core_hw_serdes_rx_lane_state_set(core_link->core_lgrp,
			sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num),
			SL_CORE_HW_SERDES_LANE_STATE_DOWN);
	}
}

int sl_core_hw_serdes_eye_upper_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_upper)
{
	int rtn;
	u32 addr;
	u8  serdes_lane_num;

	if (!sl_core_ldev_serdes_is_ready(core_lgrp->core_ldev))
		return -EIO;

	serdes_lane_num = sl_core_hw_serdes_rx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x7, serdes_lane_num, core_lgrp);

	SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_addr,
		core_lgrp->serdes.dt.dev_id, serdes_lane_num, addr, eye_upper);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"eye upper get (serdes_lane_num = %u, asic_lane_num = %u, eye_upper = %u)",
		serdes_lane_num, asic_lane_num, *eye_upper);

	rtn = 0;

out:
	return rtn;
}

int sl_core_hw_serdes_eye_lower_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_lower)
{
	int rtn;
	u32 addr;
	u8  serdes_lane_num;

	if (!sl_core_ldev_serdes_is_ready(core_lgrp->core_ldev))
		return -EIO;

	serdes_lane_num = sl_core_hw_serdes_rx_serdes_lane_num_get(core_lgrp, asic_lane_num);

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x8, serdes_lane_num, core_lgrp);

	SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_addr,
		core_lgrp->serdes.dt.dev_id, serdes_lane_num, addr, eye_lower);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"eye lower get (serdes_lane_num = %u, asic_lane_num = %u, eye_lower = %u)",
		serdes_lane_num, asic_lane_num, *eye_lower);

	rtn = 0;

out:
	return rtn;
}

void sl_core_hw_serdes_tx_lane_state_set(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u32 state)
{
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"tx lane state set (asic_lane_num = %u, state = %u %s)",
		asic_lane_num, state, sl_core_serdes_lane_state_str(state));

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->serdes.lane_state[asic_lane_num].tx = state;
	spin_unlock(&core_lgrp->data_lock);
}

u32 sl_core_hw_serdes_tx_lane_state_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num)
{
	u32 state;

	spin_lock(&core_lgrp->data_lock);
	state = core_lgrp->serdes.lane_state[asic_lane_num].tx;
	spin_unlock(&core_lgrp->data_lock);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"tx lane state get (asic_lane_num = %u, state = %u %s)",
		asic_lane_num, state, sl_core_serdes_lane_state_str(state));

	return state;
}

void sl_core_hw_serdes_rx_lane_state_set(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u32 state)
{
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"rx lane state set (asic_lane_num = %u, state = %u %s)",
		asic_lane_num, state, sl_core_serdes_lane_state_str(state));

	spin_lock(&core_lgrp->data_lock);
	core_lgrp->serdes.lane_state[asic_lane_num].rx = state;
	spin_unlock(&core_lgrp->data_lock);
}

u32 sl_core_hw_serdes_rx_lane_state_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num)
{
	u32 state;

	spin_lock(&core_lgrp->data_lock);
	state = core_lgrp->serdes.lane_state[asic_lane_num].rx;
	spin_unlock(&core_lgrp->data_lock);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"rx lane state get (asic_lane_num = %u, state = %u %s)",
		asic_lane_num, state, sl_core_serdes_lane_state_str(state));

	return state;
}

int sl_core_hw_serdes_tx_lane_is_lol(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, bool *is_tx_lol)
{
	int                         rtn;
	u8                          serdes_lane_map;
	struct sl_media_jack_signal media_signal;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "tx lane is lol (asic_lane_num = %u)", asic_lane_num);

	serdes_lane_map = BIT(sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num));

	rtn = sl_media_jack_signal_cache_get(core_lgrp->core_ldev->num, core_lgrp->num,
					     serdes_lane_map, &media_signal);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "signal cache get failed [%d]", rtn);
		return rtn;
	}

	*is_tx_lol = media_signal.tx.lol_map & serdes_lane_map;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "tx lane lol check (is_tx_lol = %s)", *is_tx_lol ? "yes" : "no");

	return 0;
}

int sl_core_hw_serdes_rx_lane_is_lol(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, bool *is_rx_lol)
{
	int                         rtn;
	u8                          serdes_lane_map;
	struct sl_media_jack_signal media_signal;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "rx lane is lol (asic_lane_num = %u)", asic_lane_num);

	serdes_lane_map = BIT(sl_core_hw_serdes_rx_serdes_lane_num_get(core_lgrp, asic_lane_num));

	rtn = sl_media_jack_signal_cache_get(core_lgrp->core_ldev->num, core_lgrp->num,
					     serdes_lane_map, &media_signal);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "signal cache get failed [%d]", rtn);
		return rtn;
	}

	*is_rx_lol = media_signal.rx.lol_map & serdes_lane_map;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "rx lane lol check (is_rx_lol = %s)", *is_rx_lol ? "yes" : "no");

	return 0;
}

int sl_core_hw_serdes_tx_lane_is_los(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, bool *is_tx_los)
{
	int                         rtn;
	u8                          serdes_lane_map;
	struct sl_media_jack_signal media_signal;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "tx lane is los (asic_lane_num = %u)", asic_lane_num);

	serdes_lane_map = BIT(sl_core_hw_serdes_tx_serdes_lane_num_get(core_lgrp, asic_lane_num));

	rtn = sl_media_jack_signal_cache_get(core_lgrp->core_ldev->num, core_lgrp->num,
					     serdes_lane_map, &media_signal);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "signal cache get failed [%d]", rtn);
		return rtn;
	}

	*is_tx_los = media_signal.tx.los_map & serdes_lane_map;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "tx lane los check (is_tx_los = %s)", *is_tx_los ? "yes" : "no");

	return 0;
}

int sl_core_hw_serdes_rx_lane_is_los(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, bool *is_rx_los)
{
	int                         rtn;
	u8                          serdes_lane_map;
	struct sl_media_jack_signal media_signal;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "rx lane is los (asic_lane_num = %u)", asic_lane_num);

	serdes_lane_map = BIT(sl_core_hw_serdes_rx_serdes_lane_num_get(core_lgrp, asic_lane_num));

	rtn = sl_media_jack_signal_cache_get(core_lgrp->core_ldev->num, core_lgrp->num,
					     serdes_lane_map, &media_signal);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "signal cache get failed [%d]", rtn);
		return rtn;
	}

	*is_rx_los = media_signal.rx.los_map & serdes_lane_map;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "rx lane los check (is_rx_los = %s)", *is_rx_los ? "yes" : "no");

	return 0;
}
