// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>

#include "sl_asic.h"

#include "sl_kconfig.h"

#include "sl_core_link.h"
#include "base/sl_core_log.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_test_fec.h"

#define LOG_NAME SL_CORE_TEST_FEC_LOG_NAME

int sl_core_test_fec_cntrs_use_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool use_test_cntrs)
{
	struct sl_core_link *link;

	link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	if (!link) {
		sl_core_log_err(link, LOG_NAME, "NULL link");
		return -EINVAL;
	}

	spin_lock(&link->fec.test_lock);
	link->fec.use_test_cntrs = use_test_cntrs;
	spin_unlock(&link->fec.test_lock);

	sl_core_log_dbg(link, LOG_NAME, "fec_cntrs_use (use_test_cntrs = %s)", use_test_cntrs ? "true" : "false");

	return 0;
}

int sl_core_test_fec_cw_cntrs_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	struct sl_core_link *link;

	link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	if (!link) {
		sl_core_log_err(link, LOG_NAME, "NULL link");
		return -EINVAL;
	}

	sl_core_log_dbg(link, LOG_NAME, "cntrs set");

	spin_lock(&link->fec.test_lock);
	link->fec.test_cntrs = *cw_cntrs;
	spin_unlock(&link->fec.test_lock);

	sl_core_log_dbg(link, LOG_NAME, "cntrs set (ucw = %llu, ccw = %llu, gcw = %llu)",
		 cw_cntrs->ucw, cw_cntrs->ccw, cw_cntrs->gcw);

	return 0;
}
