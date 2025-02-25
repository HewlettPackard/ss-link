// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include "sl_kconfig.h"

#if defined(CONFIG_SL_FRAMEWORK_TEST)

#include <linux/sl_test.h>

#include "sl_ldev.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "sl_log.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"

#include "test/sl_ctl_test.h"
#include "test/sl_core_test.h"
#include "test/sl_core_test_an.h"
#include "test/sl_core_test_fec.h"
#include "test/sl_media_test.h"

int sl_test_ctl(struct sl_ldev *ldev, u64 lgrp_map, u64 link_map, u32 test_num, u32 flags)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn)
		return rtn;

	return sl_ctl_test_exec(ldev->num, lgrp_map, link_map, test_num, flags);
}
EXPORT_SYMBOL(sl_test_ctl);

int sl_test_core(struct sl_ldev *ldev, u64 lgrp_map, u64 link_map, u32 test_num, u32 flags)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn)
		return rtn;

	return sl_core_test_exec(ldev->num, lgrp_map, link_map, test_num, flags);
}
EXPORT_SYMBOL(sl_test_core);

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

int sl_test_media(struct sl_ldev *ldev, u64 lgrp_map, u32 test_num, u32 flags)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn)
		return rtn;

	return sl_test_media_test_exec(ldev->num, lgrp_map, test_num, flags);
}
EXPORT_SYMBOL(sl_test_media);

#endif /* CONFIG_SL_FRAMEWORK_TEST */
