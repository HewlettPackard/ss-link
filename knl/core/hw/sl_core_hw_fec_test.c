// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_kconfig.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_fec_test.h"

#define LOG_NAME SL_CORE_TEST_FEC_LOG_NAME

int sl_core_hw_test_fec_cw_cntrs_get(struct sl_core_link *core_link, struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	sl_core_log_dbg(core_link, LOG_NAME, "cntrs_get");

	spin_lock(&core_link->fec.test_lock);
	core_link->fec.mock_cntr.ucw += core_link->fec.test_cntrs.ucw;
	core_link->fec.mock_cntr.ccw += core_link->fec.test_cntrs.ccw;
	core_link->fec.mock_cntr.gcw += core_link->fec.test_cntrs.gcw;
	*cw_cntrs = core_link->fec.mock_cntr;
	spin_unlock(&core_link->fec.test_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"cntrs_get (ucw = %llu, ccw = %llu, gcw = %llu)",
		 cw_cntrs->ucw, cw_cntrs->ccw, cw_cntrs->gcw);

	return 0;
}
