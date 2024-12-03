// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_core_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "hw/sl_core_hw_settings.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_pmi.h"
#include "data/sl_core_data_lgrp.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "uapi/sl_media.h"
#include "uapi/sl_link.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

static int sl_core_hw_serdes_init(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	if (core_lgrp->serdes.state != SL_CORE_LGRP_SERDES_STATE_INIT)
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "init");

	/* set lane map back to straight */
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD190);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x00000000);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD191);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x01010000);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD192);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x02020000);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD193);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x03030000);
#ifdef BUILDSYS_FRAMEWORK_ROSETTA
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD194);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x04040000);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD195);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x05050000);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD196);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x06060000);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, 0x08FFD197);
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, 0x07070000);
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_swizzles(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	if (core_lgrp->serdes.is_swizzled)
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "swizzles");

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD190 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[0].tx_source + ((core_lgrp->num & BIT(0)) * 4), 8, 0x1F00);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD190 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[0].rx_source + ((core_lgrp->num & BIT(0)) * 4), 0, 0x001F);

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD191 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[1].tx_source + ((core_lgrp->num & BIT(0)) * 4), 8, 0x1F00);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD191 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[1].rx_source + ((core_lgrp->num & BIT(0)) * 4), 0, 0x001F);

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD192 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[2].tx_source + ((core_lgrp->num & BIT(0)) * 4), 8, 0x1F00);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD192 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[2].rx_source + ((core_lgrp->num & BIT(0)) * 4), 0, 0x001F);

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD193 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[3].tx_source + ((core_lgrp->num & BIT(0)) * 4), 8, 0x1F00);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		0xD193 + ((core_lgrp->num & BIT(0)) * 4),
		core_lgrp->serdes.dt.lane_info[3].rx_source + ((core_lgrp->num & BIT(0)) * 4), 0, 0x001F);

	core_lgrp->serdes.is_swizzled = true;

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_start(struct sl_core_lgrp *core_lgrp, u32 clocking)
{
	int rtn;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "start");

	if (core_lgrp->num & BIT(0)) {
		rtn = sl_core_hw_serdes_swizzles(core_lgrp);
		if (rtn) {
			core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
			sl_core_log_err(core_lgrp, LOG_NAME,
				"hw_serdes_swizzles failed [%d]", rtn);
			return -EIO;
		}
		rtn = sl_core_hw_serdes_fw_info_get(core_lgrp);
		if (rtn != 0) {
			core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
			sl_core_log_err(core_lgrp, LOG_NAME,
				"hw_serdes_fw_info_get failed [%d]", rtn);
			return -EIO;
		}
		core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_READY;
		return 0;
	}

	rtn = sl_core_hw_serdes_init(core_lgrp);
	if (rtn != 0) {
		core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
		sl_core_log_err(core_lgrp, LOG_NAME,
			"hw_serdes_init failed [%d]", rtn);
		return -EIO;
	}
	rtn = sl_core_hw_serdes_fw_load(core_lgrp);
	if (rtn != 0) {
		core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
		sl_core_log_err(core_lgrp, LOG_NAME,
			"hw_serdes_fw_load failed [%d]", rtn);
		return -EIO;
	}
	rtn = sl_core_hw_serdes_fw_info_get(core_lgrp);
	if (rtn != 0) {
		core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
		sl_core_log_err(core_lgrp, LOG_NAME,
			"hw_serdes_fw_info_get failed [%d]", rtn);
		return -EIO;
	}
	rtn = sl_core_hw_serdes_core_init(core_lgrp);
	if (rtn != 0) {
		core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
		sl_core_log_err(core_lgrp, LOG_NAME,
			"hw_serdes_core_init failed [%d]", rtn);
		return -EIO;
	}
	rtn = sl_core_hw_serdes_core_pll(core_lgrp, clocking);
	if (rtn != 0) {
		core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
		sl_core_log_err(core_lgrp, LOG_NAME,
			"hw_serdes_core_pll failed [%d]", rtn);
		return -EIO;
	}
	rtn = sl_core_hw_serdes_swizzles(core_lgrp);
	if (rtn) {
		core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_ERROR;
		sl_core_log_err(core_lgrp, LOG_NAME,
			"hw_serdes_swizzles failed [%d]", rtn);
		return -EIO;
	}

	core_lgrp->serdes.state = SL_CORE_LGRP_SERDES_STATE_READY;

	return 0;
}

static int sl_core_hw_serdes_link_up_an_settings(struct sl_core_link *core_link)
{
	struct sl_media_lgrp *media_lgrp;

	sl_core_log_dbg(core_link, LOG_NAME, "link up an settings");

	media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);
	if (sl_media_lgrp_is_cable_not_supported(media_lgrp))
		sl_core_log_warn(core_link, LOG_NAME,
			"cable not supported - using default serdes settings");

	sl_media_lgrp_media_serdes_settings_get(core_link->core_lgrp->core_ldev->num,
					core_link->core_lgrp->num, &core_link->serdes.media_serdes_settings);

	core_link->serdes.core_serdes_settings.dfe      = SL_CORE_HW_SERDES_DFE_DISABLE;
	core_link->serdes.core_serdes_settings.scramble = SL_CORE_HW_SERDES_SCRAMBLE_DISABLE;
	core_link->serdes.core_serdes_settings.osr      = SL_CORE_HW_SERDES_OSR_OSX42P5;
	core_link->serdes.core_serdes_settings.encoding = SL_CORE_HW_SERDES_ENCODING_NRZ;
// FIXME: might need to worry about third party
	core_link->serdes.core_serdes_settings.clocking = SL_CORE_HW_SERDES_CLOCKING_85;
	core_link->serdes.core_serdes_settings.width    = SL_CORE_HW_SERDES_WIDTH_40;

	switch (core_link->core_lgrp->config.furcation) {
	case SL_MEDIA_FURCATION_X1:
		core_link->serdes.lane_map = BIT(0);
		break;
	case SL_MEDIA_FURCATION_X2:
		core_link->serdes.lane_map = BIT(0) << (core_link->num * 2);
		break;
	case SL_MEDIA_FURCATION_X4:
		core_link->serdes.lane_map = BIT(core_link->num);
		break;
	}
	core_link->serdes.lane_map <<= (4 * (core_link->core_lgrp->num & BIT(0)));
	sl_core_log_dbg(core_link, LOG_NAME, "link up an settings (lane_map = 0x%02X)",
		core_link->serdes.lane_map);

	return 0;
}

int sl_core_hw_serdes_link_up_an(struct sl_core_link *core_link)
{
	int rtn;

	if (!SL_PLATFORM_IS_HARDWARE(core_link->core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_link, LOG_NAME, "link up an");

	rtn = sl_core_hw_serdes_link_up_an_settings(core_link);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME,
			"hw_serdes_link_up_an_settings failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_core_hw_serdes_start(core_link->core_lgrp, core_link->serdes.core_serdes_settings.clocking);
	if (rtn) {
		sl_core_log_err(core_link, LOG_NAME,
			"hw_serdes_start failed [%d]", rtn);
		return -EIO;
	}

	rtn = sl_core_hw_serdes_lanes_up(core_link, SL_CORE_HW_SERDES_NO_CHECK);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"hw_serdes_lanes_up failed [%d]", rtn);
		return -EIO;
	}

	return 0;
}

static int sl_core_hw_serdes_link_up_settings(struct sl_core_link *core_link)
{
	struct sl_media_lgrp *media_lgrp;

	sl_core_log_dbg(core_link, LOG_NAME,
		"link up settings (config.flags = 0x%X)", core_link->config.flags);

	media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);
	if (sl_media_lgrp_is_cable_not_supported(media_lgrp))
		sl_core_log_warn(core_link, LOG_NAME,
			"cable not supported - using default serdes settings");

	if (is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_SERDES_LOOPBACK_ENABLE)) {
		sl_core_log_dbg(core_link, LOG_NAME, "link up settings loopback");
		SL_CORE_HW_SERDES_LOOPBACK_SERDES_SETTINGS_SET(core_link);
	} else {
		sl_core_log_dbg(core_link, LOG_NAME, "link up settings normal");
		sl_media_lgrp_media_serdes_settings_get(core_link->core_lgrp->core_ldev->num,
							core_link->core_lgrp->num, &core_link->serdes.media_serdes_settings);
	}

	sl_core_log_dbg(core_link, LOG_NAME,
		"link up settings (pcs_mode = %u)", core_link->pcs.settings.pcs_mode);
	switch (core_link->pcs.settings.pcs_mode) {
	case SL_CORE_HW_PCS_MODE_BJ_100G:
		core_link->serdes.core_serdes_settings.encoding = SL_CORE_HW_SERDES_ENCODING_NRZ;
		core_link->serdes.core_serdes_settings.clocking = SL_CORE_HW_SERDES_CLOCKING_82P5;
		core_link->serdes.core_serdes_settings.width    = SL_CORE_HW_SERDES_WIDTH_40;
		core_link->serdes.core_serdes_settings.osr      = SL_CORE_HW_SERDES_OSR_OSX2;
		break;
	case SL_CORE_HW_PCS_MODE_CD_50G:
	case SL_CORE_HW_PCS_MODE_CD_100G:
	case SL_CORE_HW_PCS_MODE_BS_200G:
		core_link->serdes.core_serdes_settings.encoding = SL_CORE_HW_SERDES_ENCODING_PAM4_NR;
		core_link->serdes.core_serdes_settings.clocking = SL_CORE_HW_SERDES_CLOCKING_85;
		core_link->serdes.core_serdes_settings.width    = SL_CORE_HW_SERDES_WIDTH_80;
		core_link->serdes.core_serdes_settings.osr      = SL_CORE_HW_SERDES_OSR_OSX2;
		break;
	case SL_CORE_HW_PCS_MODE_CK_100G:
	case SL_CORE_HW_PCS_MODE_CK_200G:
	case SL_CORE_HW_PCS_MODE_CK_400G:
		core_link->serdes.core_serdes_settings.encoding = SL_CORE_HW_SERDES_ENCODING_PAM4_NR;
		core_link->serdes.core_serdes_settings.clocking = SL_CORE_HW_SERDES_CLOCKING_85;
		core_link->serdes.core_serdes_settings.width    = SL_CORE_HW_SERDES_WIDTH_160;
		core_link->serdes.core_serdes_settings.osr      = SL_CORE_HW_SERDES_OSR_OSX1;
		break;
	}
	if (is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_EXTENDED_REACH_FORCE))
		core_link->serdes.core_serdes_settings.encoding = SL_CORE_HW_SERDES_ENCODING_PAM4_ER;

	core_link->serdes.core_serdes_settings.dfe = SL_CORE_HW_SERDES_DFE_ENABLE;
	if (sl_media_jack_cable_shift_state_get(media_lgrp->media_jack) == SL_MEDIA_JACK_CABLE_SHIFT_STATE_DOWNSHIFTED)
		core_link->serdes.core_serdes_settings.scramble = SL_CORE_HW_SERDES_SCRAMBLE_DISABLE;
	else
		core_link->serdes.core_serdes_settings.scramble = SL_CORE_HW_SERDES_SCRAMBLE_ENABLE;

	switch (core_link->core_lgrp->config.furcation) {
	case SL_MEDIA_FURCATION_X1:
		core_link->serdes.lane_map = (BIT(0) | BIT(1) | BIT(2) | BIT(3));
		break;
	case SL_MEDIA_FURCATION_X2:
		core_link->serdes.lane_map = ((BIT(0) | BIT(1)) << (core_link->num * 2));
		break;
	case SL_MEDIA_FURCATION_X4:
		core_link->serdes.lane_map = BIT(core_link->num);
		break;
	}
	core_link->serdes.lane_map <<= (4 * (core_link->core_lgrp->num & BIT(0)));
	sl_core_log_dbg(core_link, LOG_NAME, "link up settings (lane_map = 0x%02X)",
		core_link->serdes.lane_map);

	core_link->core_lgrp->serdes.eye_limits[0].low  = 15;
	core_link->core_lgrp->serdes.eye_limits[0].high = 60;
	core_link->core_lgrp->serdes.eye_limits[1].low  = 15;
	core_link->core_lgrp->serdes.eye_limits[1].high = 60;
	core_link->core_lgrp->serdes.eye_limits[2].low  = 15;
	core_link->core_lgrp->serdes.eye_limits[2].high = 60;
	core_link->core_lgrp->serdes.eye_limits[3].low  = 15;
	core_link->core_lgrp->serdes.eye_limits[3].high = 60;
	if (core_link->pcs.settings.pcs_mode == SL_CORE_HW_PCS_MODE_BJ_100G) {
		core_link->core_lgrp->serdes.eye_limits[0].low  = 25;
		core_link->core_lgrp->serdes.eye_limits[0].high = 150;
		core_link->core_lgrp->serdes.eye_limits[1].low  = 25;
		core_link->core_lgrp->serdes.eye_limits[1].high = 150;
		core_link->core_lgrp->serdes.eye_limits[2].low  = 25;
		core_link->core_lgrp->serdes.eye_limits[2].high = 150;
		core_link->core_lgrp->serdes.eye_limits[3].low  = 25;
		core_link->core_lgrp->serdes.eye_limits[3].high = 150;
	}

	/* test settings */
	if (core_link->serdes.use_test_settings) {
		sl_core_log_warn(core_link, LOG_NAME, "using test serdes settings");
		// media settings
		core_link->serdes.media_serdes_settings.pre1    = core_link->serdes.test_settings.pre1;
		core_link->serdes.media_serdes_settings.pre2    = core_link->serdes.test_settings.pre2;
		core_link->serdes.media_serdes_settings.pre3    = core_link->serdes.test_settings.pre3;
		core_link->serdes.media_serdes_settings.cursor  = core_link->serdes.test_settings.cursor;
		core_link->serdes.media_serdes_settings.post1   = core_link->serdes.test_settings.post1;
		core_link->serdes.media_serdes_settings.post2   = core_link->serdes.test_settings.post2;
		core_link->serdes.media_serdes_settings.media   = core_link->serdes.test_settings.media;
		// core settings
		core_link->serdes.core_serdes_settings.osr      = core_link->serdes.test_settings.osr;
		core_link->serdes.core_serdes_settings.encoding = core_link->serdes.test_settings.encoding;
		core_link->serdes.core_serdes_settings.clocking = core_link->serdes.test_settings.clocking;
		core_link->serdes.core_serdes_settings.width    = core_link->serdes.test_settings.width;
		core_link->serdes.core_serdes_settings.dfe      = core_link->serdes.test_settings.dfe;
		core_link->serdes.core_serdes_settings.scramble = core_link->serdes.test_settings.scramble;
		// FIXME: options?
	}

	return 0;
}

int sl_core_hw_serdes_link_up(struct sl_core_link *core_link)
{
	int rtn;

	if (!SL_PLATFORM_IS_HARDWARE(core_link->core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_link, LOG_NAME, "link up");

	rtn = sl_core_hw_serdes_link_up_settings(core_link);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME,
			"hw_serdes_link_up_settings failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_core_hw_serdes_start(core_link->core_lgrp, core_link->serdes.core_serdes_settings.clocking);
	if (rtn) {
		sl_core_log_err(core_link, LOG_NAME,
			"hw_serdes_start failed [%d]", rtn);
		return -EIO;
	}

	rtn = sl_core_hw_serdes_lanes_up(core_link, SL_CORE_HW_SERDES_CHECK);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"hw_serdes_lanes_up failed [%d]", rtn);
		return -EIO;
	}

	return 0;
}

static int sl_core_hw_serdes_link_down_settings(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "link down settings");

	switch (core_link->core_lgrp->config.furcation) {
	case SL_MEDIA_FURCATION_X1:
		core_link->serdes.lane_map = (BIT(0) | BIT(1) | BIT(2) | BIT(3));
		break;
	case SL_MEDIA_FURCATION_X2:
		core_link->serdes.lane_map = ((BIT(0) | BIT(1)) << (core_link->num * 2));
		break;
	case SL_MEDIA_FURCATION_X4:
		core_link->serdes.lane_map = BIT(core_link->num);
		break;
	}
	core_link->serdes.lane_map <<= (4 * (core_link->core_lgrp->num & BIT(0)));
	sl_core_log_dbg(core_link, LOG_NAME, "link down settings (lane_map = 0x%X)",
		core_link->serdes.lane_map);

	return 0;
}

void sl_core_hw_serdes_link_down(struct sl_core_link *core_link)
{
	int rtn;

	if (!SL_PLATFORM_IS_HARDWARE(core_link->core_lgrp->core_ldev))
		return;

	sl_core_log_dbg(core_link, LOG_NAME, "link down");

	rtn = sl_core_hw_serdes_link_down_settings(core_link);
	if (rtn)
		sl_core_log_warn(core_link, LOG_NAME,
			"hw_serdes_link_down_settings failed [%d]", rtn);

	sl_core_hw_serdes_lanes_down(core_link);

	memset(&(core_link->serdes.core_serdes_settings), 0, sizeof(struct sl_core_serdes_settings));
	memset(&(core_link->serdes.media_serdes_settings), 0,
			sizeof(struct sl_media_serdes_settings));
}
