// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

#include "sl_asic.h"
#include "sl_core_link.h"
#include "sl_core_lgrp.h"
#include "sl_core_ldev.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes_core.h"
#include "hw/sl_core_hw_serdes_link.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "hw/sl_core_hw_settings.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "uapi/sl_media.h"
#include "uapi/sl_link.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

#define SL_CORE_HW_SERDES_STATE_DOWN 0
#define SL_CORE_HW_SERDES_STATE_UP   1

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
		sl_core_log_err_trace(core_link, LOG_NAME,
			"serdes_link_up_an_settings failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_core_hw_serdes_core_pll(core_link->core_lgrp,
		core_link->serdes.core_serdes_settings.clocking);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"serdes_core_pll failed [%d]", rtn);
		return -EIO;
	}

	rtn = sl_core_hw_serdes_lanes_up(core_link, SL_CORE_HW_SERDES_NO_CHECK);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"serdes_lanes_up failed [%d]", rtn);
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

	/*
	 * FIXME: remove it when this feature is needed
	 */
#if 0
	if (sl_core_hw_serdes_state_get(core_link) == SL_CORE_HW_SERDES_STATE_UP) {
		sl_core_log_dbg(core_link, LOG_NAME, "serdes already up");
		return 0;
	}
#endif

	rtn = sl_core_hw_serdes_link_up_settings(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"serdes_link_up_settings failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_core_hw_serdes_core_pll(core_link->core_lgrp,
		core_link->serdes.core_serdes_settings.clocking);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"serdes_core_pll failed [%d]", rtn);
		return -EIO;
	}

	rtn = sl_core_hw_serdes_lanes_up(core_link, SL_CORE_HW_SERDES_CHECK);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"serdes_lanes_up failed [%d]", rtn);
		return -EIO;
	}

	sl_core_hw_serdes_state_set(core_link, SL_CORE_HW_SERDES_STATE_UP);

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

	/*
	 * FIXME: remove it when this feature is needed
	 */
#if 0
	/*if (sl_core_link_policy_is_keep_serdes_up_set(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "link down - keeping serdes up");
		return;
	}*/
#endif

	rtn = sl_core_hw_serdes_link_down_settings(core_link);
	if (rtn)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"serdes_link_down_settings failed [%d]", rtn);

	sl_core_hw_serdes_lanes_down(core_link);

	memset(&(core_link->serdes.core_serdes_settings), 0,
		sizeof(struct sl_core_serdes_settings));
	memset(&(core_link->serdes.media_serdes_settings), 0,
		sizeof(struct sl_media_serdes_settings));

	sl_core_hw_serdes_state_set(core_link, SL_CORE_HW_SERDES_STATE_DOWN);
}
