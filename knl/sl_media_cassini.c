// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include <linux/sl_media.h>

#include "base/sl_media_log.h"
#include "sl_lgrp.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack_cassini.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_MEDIA_LOG_NAME

int sl_media_cable_insert(struct sl_lgrp *lgrp, u8 *eeprom_page0, u8 *eeprom_page1, u32 flags)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cable insert fail");
		return rtn;
	}
	if (!eeprom_page0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL eeprom");
		return -EINVAL;
	}

	/*
	 * hardcoding 0 for now since there is only one jack. Might change in future
	 */
	return sl_media_jack_cable_insert(lgrp->ldev_num, lgrp->num, 0, eeprom_page0,
					  eeprom_page1, flags);
}
EXPORT_SYMBOL(sl_media_cable_insert);

int sl_media_cable_remove(struct sl_lgrp *lgrp)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cable remove fail");
		return rtn;
	}

	/*
	 * hardcoding 0 for now since there is only one jack. Might change in future
	 */
	return sl_media_jack_cable_remove(lgrp->ldev_num, lgrp->num, 0);
}
EXPORT_SYMBOL(sl_media_cable_remove);
