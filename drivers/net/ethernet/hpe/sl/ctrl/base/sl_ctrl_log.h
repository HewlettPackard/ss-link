/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LOG_H_
#define _SL_CTRL_LOG_H_

#include "sl_log.h"

#define SL_CTRL_LOG_BLOCK           "ctrl"

#define SL_CTRL_MEDIA_LOG_NAME      "media"
#define SL_CTRL_LDEV_LOG_NAME       "ldev"
#define SL_CTRL_LGRP_LOG_NAME       "lgrp"
#define SL_CTRL_LGRP_NOTIF_LOG_NAME "lgrp-ntf"
#define SL_CTRL_LINK_LOG_NAME       "link"
#define SL_CTRL_LINK_FEC_LOG_NAME   "link-fec"
#define SL_CTRL_MAC_LOG_NAME        "mac"
#define SL_CTRL_LLR_LOG_NAME        "llr"
#define SL_CTRL_TEST_FEC_LOG_NAME   "test-fec"

#define sl_ctrl_log_dbg(_ptr, _name, _text, ...) \
	sl_log_dbg((_ptr), SL_CTRL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctrl_log_info(_ptr, _name, _text, ...) \
	sl_log_info((_ptr), SL_CTRL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctrl_log_warn(_ptr, _name, _text, ...) \
	sl_log_warn((_ptr), SL_CTRL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctrl_log_warn_trace(_ptr, _name, _text, ...) \
	sl_log_warn_trace((_ptr), SL_CTRL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctrl_log_err(_ptr, _name, _text, ...) \
	sl_log_err((_ptr), SL_CTRL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_ctrl_log_err_trace(_ptr, _name, _text, ...) \
	sl_log_err_trace((_ptr), SL_CTRL_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)

#endif /* _SL_CTRL_LOG_H_ */
