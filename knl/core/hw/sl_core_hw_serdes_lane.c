// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/delay.h>

#include "sl_kconfig.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_uc_ram.h"
#include "hw/sl_core_hw_settings.h"
#include "data/sl_core_data_lgrp.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

#define SL_MAX_SERDES_LANES 8

u8 sl_core_hw_serdes_rx_asic_lane_num_get(struct sl_core_lgrp *core_lgrp, u8 serdes_lane_num)
{
	u8 asic_lane_num;

	for (asic_lane_num = 0; asic_lane_num < SL_MAX_LANES; ++asic_lane_num)
		if (core_lgrp->serdes.dt.lane_info[asic_lane_num].rx_source +
				(4 * (core_lgrp->num & BIT(0))) == serdes_lane_num)
			return asic_lane_num;

	sl_core_log_err(core_lgrp, LOG_NAME,
		"RX lane num not found (serdes_lane_num = %u)", serdes_lane_num);
	return 0;
}

u8 sl_core_hw_serdes_tx_asic_lane_num_get(struct sl_core_lgrp *core_lgrp, u8 serdes_lane_num)
{
	u8 asic_lane_num;

	for (asic_lane_num = 0; asic_lane_num < SL_MAX_LANES; ++asic_lane_num)
		if (core_lgrp->serdes.dt.lane_info[asic_lane_num].tx_source +
				(4 * (core_lgrp->num & BIT(0))) == serdes_lane_num)
			return asic_lane_num;

	sl_core_log_err(core_lgrp, LOG_NAME,
		"TX lane num not found (serdes_lane_num = %u)", serdes_lane_num);
	return 0;
}

u8 sl_core_hw_serdes_rx_serdes_lane_num_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num)
{
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"RX lane get (asic_lane_num = %u, serdes_lane_num = %lu)", asic_lane_num,
		core_lgrp->serdes.dt.lane_info[asic_lane_num].rx_source + (4 * (core_lgrp->num & BIT(0))));

	return core_lgrp->serdes.dt.lane_info[asic_lane_num].rx_source + (4 * (core_lgrp->num & BIT(0)));
}

u8 sl_core_hw_serdes_tx_serdes_lane_num_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num)
{
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"TX lane get (asic_lane_num = %u, serdes_lane_num = %lu)", asic_lane_num,
		core_lgrp->serdes.dt.lane_info[asic_lane_num].tx_source + (4 * (core_lgrp->num & BIT(0))));

	return core_lgrp->serdes.dt.lane_info[asic_lane_num].tx_source + (4 * (core_lgrp->num & BIT(0)));
}

int sl_core_hw_serdes_lane_pre1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre1)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD135, 7, 7, &data16); /* TXFIR_TAP2_COEFF */

	*pre1 = (s16)(data16 << 7) >> 7;

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

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD134, 7, 7, &data16); /* TXFIR_TAP1_COEFF */

	*pre2 = (s16)(data16 << 7) >> 7;

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

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD133, 7, 7, &data16); /* TXFIR_TAP0_COEFF */

	*pre3 = (s16)(data16 << 7) >> 7;

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

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD136, 7, 7, &data16); /* TXFIR_TAP3_COEFF */

	*cursor = (s16)(data16 << 7) >> 7;

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

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD137, 7, 7, &data16); /* TXFIR_TAP4_COEFF */

	*post1 = (s16)(data16 << 7) >> 7;

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

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD138, 7, 7, &data16); /* TXFIR_TAP5_COEFF */

	*post2 = (s16)(data16 << 7) >> 7;

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
		serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
			0, 0xD590,  0, 15, &data16);
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"encoding get (serdes_lane_num = %u, encoding = 0x%X)",
			serdes_lane_num, data16);
		if (data16)
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

	config |= ((core_serdes_settings->scramble & 0x1) << 8);

	switch (core_serdes_settings->encoding) {
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

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1AD, 0, 0, &data16); /* LANE_CFG_FWAPI_DATA0 */

	*dfe = ((data16 >> 2) & 0x1);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_scramble_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *scramble)
{
	int rtn;
	u16 data16;
	u8  serdes_lane_num;

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1AD, 0, 0, &data16); /* LANE_CFG_FWAPI_DATA0 */

	*scramble = ((data16 >> 8) & 0x1);

	return 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_lane_up_clock_align(struct sl_core_lgrp *core_lgrp, u8 serdes_lane_num)
{
	int rtn;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up clock align (serdes_lane_num = %u)", serdes_lane_num);

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0D3, 0,  7, 0x0080);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A5, 0, 11, 0x0800);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A5, 0, 10, 0x0400);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A5, 0,  4, 0x0010);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A5, 0,  3, 0x0008);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A0, 0,  0, 0x0001);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A0, 0,  1, 0x0002);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A0, 0,  2, 0x0004);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A0, 0, 12, 0x7000);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0, 0xD0A5, 0, 11, 0x0800);

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
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0x0097,  0, 0, &data16);
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
	u8                   x;
	u32                  port;
	u64                  status;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = core_link->core_lgrp;

	port = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_lgrp, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up tx check (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	for (x = 0; x < LANE_UP_TX_CHECK_NUM_TRIES; ++x) {
		if (sl_core_link_is_canceled_or_timed_out(core_link)) {
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

	sl_core_log_err(core_lgrp, LOG_NAME,
		"lane up tx check fail (lane = %u, status = 0x%llX)", asic_lane_num, status);
	return -EIO;
}

#define LANE_UP_RX_CHECK_NUM_TRIES        20
#define LANE_UP_RX_CHECK_DATA_VALID_BIT   BIT(12)
#define LANE_UP_RX_CHECK_CLOCK_VALID_BIT  BIT(8)
#define LANE_UP_RX_CHECK_LOCK_BIT         BIT(4)
#define LANE_UP_RX_CHECK_BITS             (LANE_UP_RX_CHECK_DATA_VALID_BIT  | \
					   LANE_UP_RX_CHECK_CLOCK_VALID_BIT | \
					   LANE_UP_RX_CHECK_LOCK_BIT)
/* wait for 6s for PAM4 and 1.5s for NRZ */
static int sl_core_hw_serdes_lane_up_rx_check(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	u8                   x;
	u32                  port;
	u64                  status;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp = core_link->core_lgrp;

	port = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_lgrp, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up rx check (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	for (x = 0; x < LANE_UP_RX_CHECK_NUM_TRIES; ++x) {
		if (sl_core_link_is_canceled_or_timed_out(core_link)) {
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

	sl_core_log_err(core_lgrp, LOG_NAME,
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

	core_lgrp = core_link->core_lgrp;

	port = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_lgrp, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up quality check (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* if link training decides extended reach, then modify eye limits */
	if (is_flag_set(core_link->core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN)) {
		sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
		if (SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_GET(data64) & 0x40) {
			SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
				0, 0xD590,  0, 15, &extended);
			if (extended) {
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

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x7,
		serdes_lane_num, core_lgrp->serdes.fw_info.lane_count,
		core_lgrp->serdes.fw_info.lane_var_ram_base,
		core_lgrp->serdes.fw_info.lane_var_ram_size,
		core_lgrp->serdes.fw_info.grp_ram_size);
	for (x = 0; x < LANE_UP_QUALITY_CHECK_NUM_TRIES; ++x) {
		if (sl_core_link_is_canceled_or_timed_out(core_link)) {
			sl_core_log_dbg(core_lgrp, LOG_NAME, "lane up quality check canceled");
			return -ECANCELED;
		}
		msleep(200);
		SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_id,
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

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x8,
		serdes_lane_num, core_lgrp->serdes.fw_info.lane_count,
		core_lgrp->serdes.fw_info.lane_var_ram_base,
		core_lgrp->serdes.fw_info.lane_var_ram_size,
		core_lgrp->serdes.fw_info.grp_ram_size);
	for (x = 0; x < LANE_UP_QUALITY_CHECK_NUM_TRIES; ++x) {
		if (sl_core_link_is_canceled_or_timed_out(core_link)) {
			sl_core_log_dbg(core_lgrp, LOG_NAME, "lane up quality check canceled");
			return -ECANCELED;
		}
		msleep(200);
		SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_id,
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

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id,
		serdes_lane_num, 0, 0xD163,  0, 0, &data16);
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up quality check (serdes_lane_num = %u, 0xD163 = 0x%X)", serdes_lane_num, data16);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id,
		serdes_lane_num, 0, 0xD175,  0, 0, &data16);
	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up quality check (serdes_lane_num = %u, 0xD175 = 0x%X)", serdes_lane_num, data16);

	return 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lanes_up(struct sl_core_link *core_link, bool check)
{
	int rtn;
	u8  lane_map;
	u8  serdes_lane_num;
	u8  asic_lane_num;

	lane_map = core_link->serdes.lane_map;

	sl_core_log_dbg(core_link, LOG_NAME, "lanes up (lane_map = 0x%02X)", lane_map);

	/* start */
	for (serdes_lane_num = 0; serdes_lane_num < SL_MAX_SERDES_LANES; ++serdes_lane_num) {
		if (((lane_map >> serdes_lane_num) & 0x1) == 0)
			continue;
		asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link->core_lgrp, serdes_lane_num);
		core_link->core_lgrp->serdes.lane_state[asic_lane_num].rx = SL_CORE_HW_SERDES_LANE_STATE_BUSY;
		rtn = sl_core_hw_serdes_lane_up_rx_setup(core_link->core_lgrp, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_setup failed [%d]", rtn);
			goto out;
		}
		asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_link->core_lgrp, serdes_lane_num);
		core_link->core_lgrp->serdes.lane_state[asic_lane_num].tx = SL_CORE_HW_SERDES_LANE_STATE_BUSY;
		rtn = sl_core_hw_serdes_lane_up_tx_setup(core_link->core_lgrp, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_setup failed [%d]", rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_rx_config(core_link->core_lgrp, core_link, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_config failed [%d]", rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_tx_config(core_link->core_lgrp, core_link, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_config failed [%d]", rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_rx_start(core_link->core_lgrp, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_start failed [%d]", rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_lane_up_tx_start(core_link->core_lgrp, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_start failed [%d]", rtn);
			goto out;
		}
	}

	/* clock align */
	for (serdes_lane_num = 0; serdes_lane_num < SL_MAX_SERDES_LANES; ++serdes_lane_num) {
		if (((lane_map >> serdes_lane_num) & 0x1) == 0)
			continue;
		rtn = sl_core_hw_serdes_lane_up_clock_align(core_link->core_lgrp, serdes_lane_num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_clock_align failed [%d]", rtn);
			goto out;
		}
	}

#if 0 // FIXME: removed for now, but leave here just in case.
	/* link training check */
	if (is_flag_set(flags, SL_CORE_LINK_CFG_FLAG_LINKTRAIN_ON)) {
		for (lane_num = 0; lane_num < SL_MAX_LANES; ++lane_num) {
			if (((lane_map >> lane_num) & 0x1) == 0)
				continue;
			rtn = sl_core_hw_serdes_lane_up_linktrain_check(core_link->core_lgrp, lane_num);
			if (rtn) {
				sl_core_log_err(core_link, LOG_NAME,
					"lane_up_linktrain_check failed [%d]", rtn);
				goto out;
			}
		}
	}
#endif

	if (check) {
		for (serdes_lane_num = 0; serdes_lane_num < SL_MAX_SERDES_LANES; ++serdes_lane_num) {
			if (((lane_map >> serdes_lane_num) & 0x1) == 0)
				continue;
			rtn = sl_core_hw_serdes_lane_up_tx_check(core_link, serdes_lane_num);
			if (rtn) {
				sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_tx_check failed [%d]", rtn);
				goto out;
			}
			rtn = sl_core_hw_serdes_lane_up_rx_check(core_link, serdes_lane_num);
			if (rtn) {
				sl_core_log_err_trace(core_link, LOG_NAME, "lane_up_rx_check failed [%d]", rtn);
				goto out;
			}
		}
		/* quality check */
		if (!is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_SERDES_LOOPBACK_ENABLE)) {
			sl_core_log_dbg(core_link, LOG_NAME, "loopback off so check quality");
			for (serdes_lane_num = 0; serdes_lane_num < SL_MAX_SERDES_LANES; ++serdes_lane_num) {
				if (((lane_map >> serdes_lane_num) & 0x1) == 0)
					continue;
				rtn = sl_core_hw_serdes_lane_up_quality_check(core_link, serdes_lane_num);
				if (rtn) {
					sl_core_log_err(core_link, LOG_NAME, "lane_up_quality_check failed [%d]", rtn);
					goto out;
				}
			}
		}
	}

	/* up */
	for (serdes_lane_num = 0; serdes_lane_num < SL_MAX_SERDES_LANES; ++serdes_lane_num) {
		if (((lane_map >> serdes_lane_num) & 0x1) == 0)
			continue;
		asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_link->core_lgrp, serdes_lane_num);
		core_link->core_lgrp->serdes.lane_state[asic_lane_num].tx = SL_CORE_HW_SERDES_LANE_STATE_UP;
		asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link->core_lgrp, serdes_lane_num);
		core_link->core_lgrp->serdes.lane_state[asic_lane_num].rx = SL_CORE_HW_SERDES_LANE_STATE_UP;
	}

	rtn = 0;
out:
	return rtn;
}

void sl_core_hw_serdes_lanes_down(struct sl_core_link *core_link)
{
	u8 lane_map;
	u8 serdes_lane_num;
	u8 asic_lane_num;

	lane_map = core_link->serdes.lane_map;

	sl_core_log_dbg(core_link, LOG_NAME, "lanes down (lane_map = 0x%02X)", lane_map);

	for (serdes_lane_num = 0; serdes_lane_num < SL_MAX_SERDES_LANES; ++serdes_lane_num) {
		if (((lane_map >> serdes_lane_num) & 0x1) == 0)
			continue;
		sl_core_hw_serdes_lane_down_tx_stop(core_link->core_lgrp, serdes_lane_num);
		asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_link->core_lgrp, serdes_lane_num);
		core_link->core_lgrp->serdes.lane_state[asic_lane_num].tx = SL_CORE_HW_SERDES_LANE_STATE_DOWN;
		sl_core_hw_serdes_lane_down_rx_stop(core_link->core_lgrp, serdes_lane_num);
		asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link->core_lgrp, serdes_lane_num);
		core_link->core_lgrp->serdes.lane_state[asic_lane_num].rx = SL_CORE_HW_SERDES_LANE_STATE_DOWN;
	}
}

int sl_core_hw_serdes_eye_upper_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_upper)
{
	int rtn;
	u32 addr;
	u8  serdes_lane_num;

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x7,
		serdes_lane_num, core_lgrp->serdes.fw_info.lane_count,
		core_lgrp->serdes.fw_info.lane_var_ram_base,
		core_lgrp->serdes.fw_info.lane_var_ram_size,
		core_lgrp->serdes.fw_info.grp_ram_size);

	SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, addr, eye_upper);

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

	serdes_lane_num = asic_lane_num + (4 * (core_lgrp->num & BIT(0)));

	addr = SL_CORE_HW_SERDES_LANE_ADDR(0x8,
		serdes_lane_num, core_lgrp->serdes.fw_info.lane_count,
		core_lgrp->serdes.fw_info.lane_var_ram_base,
		core_lgrp->serdes.fw_info.lane_var_ram_size,
		core_lgrp->serdes.fw_info.grp_ram_size);

	SL_CORE_HW_UC_RAM_RD8(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, addr, eye_lower);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"eye lower get (serdes_lane_num = %u, asic_lane_num = %u, eye_lower = %u)",
		serdes_lane_num, asic_lane_num, *eye_lower);

	rtn = 0;

out:
	return rtn;
}
