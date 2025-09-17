// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include <linux/sl_fec.h>

#include "sl_log.h"
#include "sl_link.h"
#include "sl_ctrl_link_fec.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_FEC_LOG_NAME

int sl_fec_info_get(struct sl_link *link, struct sl_fec_info *fec_info)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "fec info get fail");
		return rtn;
	}
	if (!fec_info) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL fec_info");
		return -EINVAL;
	}

	return sl_ctrl_link_fec_info_get(link->ldev_num, link->lgrp_num, link->num, fec_info);
}
EXPORT_SYMBOL(sl_fec_info_get);

int sl_fec_tail_get(struct sl_link *link, struct sl_fec_tail *fec_tail)
{
	int rtn;

	rtn = sl_link_check(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "fec tail get fail");
		return rtn;
	}
	if (!fec_tail) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL fec_tail");
		return -EINVAL;
	}

	return sl_ctrl_link_fec_tail_get(link->ldev_num, link->lgrp_num, link->num, fec_tail);
}
EXPORT_SYMBOL(sl_fec_tail_get);

int sl_fec_ber_calc(struct sl_fec_info *fec_info, struct sl_ber *ucw_ber, struct sl_ber *ccw_ber)
{
	if (!fec_info) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL fec_info");
		return -EINVAL;
	}
	if (!ucw_ber) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ucw_ber");
		return -EINVAL;
	}
	if (!ccw_ber) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "NULL ccw_ber");
		return -EINVAL;
	}

	return sl_ctrl_link_fec_ber_calc(fec_info, ucw_ber, ccw_ber);
}
EXPORT_SYMBOL(sl_fec_ber_calc);
