// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "test/sl_core_test_serdes.h"

#define LOG_NAME SL_CORE_TEST_SERDES_LOG_NAME

int sl_core_test_serdes_settings_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
				     s16 pre1, s16 pre2, s16 pre3, s16 cursor,
				     s16 post1, s16 post2, u16 media, u16 osr, u16 encoding,
				     u16 clocking, u16 width, u16 dfe, u16 scramble, u32 options)
{
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	if (!core_link) {
		sl_core_log_err(NULL, LOG_NAME,
			"serdes settings set NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -ENOMEM;
	}

	sl_core_log_dbg(core_link, LOG_NAME,
		"serdes settings set (media = %d, clocking = %u)",
		media, clocking);

	core_link->serdes.test_settings.pre1     = pre1;
	core_link->serdes.test_settings.pre2     = pre2;
	core_link->serdes.test_settings.pre3     = pre3;
	core_link->serdes.test_settings.cursor   = cursor;
	core_link->serdes.test_settings.post1    = post1;
	core_link->serdes.test_settings.post2    = post2;
	core_link->serdes.test_settings.media    = media;

	core_link->serdes.test_settings.osr      = osr;
	core_link->serdes.test_settings.encoding = encoding;
	core_link->serdes.test_settings.clocking = clocking;
	core_link->serdes.test_settings.width    = width;
	core_link->serdes.test_settings.dfe      = dfe;
	core_link->serdes.test_settings.scramble = scramble;

	core_link->serdes.test_settings.options  = options;

	core_link->serdes.use_test_settings = true;

	return 0;
}

int sl_core_test_serdes_settings_unset(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	if (!core_link) {
		sl_core_log_err(NULL, LOG_NAME,
			"unset link doesn't exist (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -ENOMEM;
	}

	sl_core_log_dbg(core_link, LOG_NAME, "serdes settings unset");

	core_link->serdes.use_test_settings = false;

	return 0;
}

const char *sl_core_test_serdes_lane_encoding_str(u16 encoding)
{
	return sl_core_serdes_lane_encoding_str(encoding);
}

const char *sl_core_test_serdes_lane_clocking_str(u16 clocking)
{
	return sl_core_serdes_lane_clocking_str(clocking);
}

const char *sl_core_test_serdes_lane_osr_str(u16 osr)
{
	return sl_core_serdes_lane_osr_str(osr);
}

const char *sl_core_test_serdes_lane_width_str(u16 width)
{
	return sl_core_serdes_lane_width_str(width);
}

int sl_core_test_serdes_lane_encoding_from_str(const char *str, u16 *encoding)
{
	if (!str || !encoding)
		return -EINVAL;

	if (!strncmp(str, "NRZ", 3)) {
		*encoding = SL_CORE_HW_SERDES_ENCODING_NRZ;
		return 0;
	}

	if (!strncmp(str, "PAM4_normal", 11)) {
		*encoding = SL_CORE_HW_SERDES_ENCODING_PAM4_NR;
		return 0;
	}

	if (!strncmp(str, "PAM4_extended", 13)) {
		*encoding = SL_CORE_HW_SERDES_ENCODING_PAM4_ER;
		return 0;
	}

	pr_err(LOG_NAME ": no match for %s\n", str);

	return -ENOENT;
}

int sl_core_test_serdes_lane_clocking_from_str(const char *str, u16 *clocking)
{
	if (!strncmp(str, "82.5/165", 8)) {
		*clocking = SL_CORE_HW_SERDES_CLOCKING_82P5;
		return 0;
	}

	if (!strncmp(str, "85/170", 6)) {
		*clocking = SL_CORE_HW_SERDES_CLOCKING_85;
		return 0;
	}

	pr_err(LOG_NAME ": no match for %s\n", str);

	return -ENOENT;
}

int sl_core_test_serdes_lane_osr_from_str(const char *str, u16 *osr)
{
	if (!str || !osr)
		return -EINVAL;

	if (!strncmp(str, "OSX1", 4)) {
		*osr = SL_CORE_HW_SERDES_OSR_OSX1;
		return 0;
	}

	if (!strncmp(str, "OSX2", 4)) {
		*osr = SL_CORE_HW_SERDES_OSR_OSX2;
		return 0;
	}

	if (!strncmp(str, "OSX4", 4)) {
		*osr = SL_CORE_HW_SERDES_OSR_OSX4;
		return 0;
	}

	if (!strncmp(str, "OSX42P5", 7)) {
		*osr = SL_CORE_HW_SERDES_OSR_OSX42P5;
		return 0;
	}

	pr_err(LOG_NAME ": no match for %s\n", str);

	return -ENOENT;
}

int sl_core_test_serdes_lane_width_from_str(const char *str, u16 *width)
{
	if (!str || !width)
		return -EINVAL;

	if (!strncmp(str, "40", 2)) {
		*width = SL_CORE_HW_SERDES_WIDTH_40;
		return 0;
	}

	if (!strncmp(str, "80", 2)) {
		*width = SL_CORE_HW_SERDES_WIDTH_80;
		return 0;
	}

	if (!strncmp(str, "160", 3)) {
		*width = SL_CORE_HW_SERDES_WIDTH_160;
		return 0;
	}

	pr_err(LOG_NAME ": no match for %s\n", str);

	return -ENOENT;
}
