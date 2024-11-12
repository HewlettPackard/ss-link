/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LOG_H_
#define _SL_CTL_LOG_H_

#include "sl_log.h"

#define SL_CTL_LOG_BLOCK           "ctl"

#define SL_CTL_LDEV_LOG_NAME       "ldev"
#define SL_CTL_LGRP_LOG_NAME       "lgrp"
#define SL_CTL_LGRP_NOTIF_LOG_NAME "lgrp-ntf"
#define SL_CTL_LINK_LOG_NAME       "link"
#define SL_CTL_LINK_FEC_LOG_NAME   "link-fec"
#define SL_CTL_MAC_LOG_NAME        "mac"
#define SL_CTL_LLR_LOG_NAME        "llr"
#define SL_CTL_TEST_FEC_LOG_NAME   "test-fec"

#define sl_ctl_log_dbg(_ptr, _name, _text, ...) \
	sl_log_dbg((_ptr), SL_CTL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctl_log_info(_ptr, _name, _text, ...) \
	sl_log_info((_ptr), SL_CTL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctl_log_warn(_ptr, _name, _text, ...) \
	sl_log_warn((_ptr), SL_CTL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctl_log_err(_ptr, _name, _text, ...) \
	sl_log_err((_ptr), SL_CTL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctl_log_err_trace(_ptr, _name, _text, ...) \
	sl_log_err_trace((_ptr), SL_CTL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)

#endif /* _SL_CTL_LOG_H_ */
