// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include <linux/sl_test.h>

#include "sl_ldev.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "sl_log.h"

#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"

#include "test/sl_core_test_an.h"
#include "test/sl_core_test_fec.h"
#include "test/sl_media_test_str.h"

int sl_test_an_fake_caps_set(struct sl_link *link, struct sl_link_caps *caps)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn)
		return rtn;

	return sl_core_test_an_caps_set(link, caps);
}
EXPORT_SYMBOL(sl_test_an_fake_caps_set);

int sl_test_fec_cntrs_use_set(struct sl_link *link, bool use_test_cntrs)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn)
		return rtn;

	return sl_core_test_fec_cntrs_use_set(link->ldev_num, link->lgrp_num, link->num, use_test_cntrs);
}
EXPORT_SYMBOL(sl_test_fec_cntrs_use_set);

int sl_test_fec_cntrs_set(struct sl_link *link, u64 ucw, u64 ccw, u64 gcw)
{
	int                              rtn;
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	rtn = sl_link_check(link);
	if (rtn)
		return rtn;

	cw_cntrs.ucw = ucw;
	cw_cntrs.ccw = ccw;
	cw_cntrs.gcw = gcw;

	return sl_core_test_fec_cw_cntrs_set(link->ldev_num, link->lgrp_num, link->num, &cw_cntrs);
}
EXPORT_SYMBOL(sl_test_fec_cntrs_set);
