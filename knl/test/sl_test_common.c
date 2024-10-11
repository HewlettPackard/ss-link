// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include "sl_kconfig.h"

#include "sl_test_common.h"
#include "test/sl_core_test_serdes.h"

int sl_test_serdes_params_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      s16 pre1, s16 pre2, s16 pre3, s16 cursor,
			      s16 post1, s16 post2, u16 media, u16 osr, u16 encoding,
			      u16 clocking, u16 width, u16 dfe, u16 scramble, u32 options)
{
	return sl_core_test_serdes_settings_set(ldev_num, lgrp_num, link_num,
			pre1, pre2, pre3, cursor, post1, post2, media, osr, encoding,
			clocking, width, dfe, scramble, options);
}
EXPORT_SYMBOL(sl_test_serdes_params_set);

int sl_test_serdes_params_unset(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_test_serdes_settings_unset(ldev_num, lgrp_num, link_num);
}
EXPORT_SYMBOL(sl_test_serdes_params_unset);
