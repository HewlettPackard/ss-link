// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_core_link.h"
#include "base/sl_core_log.h"
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
			"set link doesn't exist (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -ENOMEM;
	}

	sl_core_log_dbg(core_link, LOG_NAME, "serdes settings set");

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
