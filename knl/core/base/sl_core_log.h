/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LOG_H_
#define _SL_CORE_LOG_H_

#include "sl_log.h"

#define SL_CORE_LOG_BLOCK                 "core"

#define SL_CORE_LDEV_LOG_NAME             "ldev"

#define SL_CORE_LGRP_LOG_NAME             "lgrp"

#define SL_CORE_LINK_LOG_NAME             "link"
#define SL_CORE_LINK_AN_LOG_NAME          "link-an"
#define SL_CORE_LINK_FEC_LOG_NAME         "link-fec"

#define SL_CORE_MAC_LOG_NAME              "mac"
#define SL_CORE_LLR_LOG_NAME              "llr"

#define SL_CORE_DATA_LDEV_LOG_NAME        "d-ldev"
#define SL_CORE_DATA_LGRP_LOG_NAME        "d-lgrp"
#define SL_CORE_DATA_LINK_LOG_NAME        "d-link"
#define SL_CORE_DATA_MAC_LOG_NAME         "d-mac"
#define SL_CORE_DATA_LLR_LOG_NAME         "d-llr"

#define SL_CORE_TIMER_LINK_LOG_NAME       "timr-lnk"
#define SL_CORE_TIMER_LLR_LOG_NAME        "timr-llr"

#define SL_CORE_WORK_LINK_LOG_NAME        "work-lnk"
#define SL_CORE_WORK_LLR_LOG_NAME         "work-llr"

#define SL_CORE_HW_LOG_NAME               "hw"
#define SL_CORE_HW_INTR_LOG_NAME          "hw-intr"
#define SL_CORE_HW_INTR_LLR_LOG_NAME      "hw-i-llr"
#define SL_CORE_HW_LINK_LOG_NAME          "hw-link"
#define SL_CORE_HW_PCS_LOG_NAME           "hw-pcs"
#define SL_CORE_HW_MAC_LOG_NAME           "hw-mac"
#define SL_CORE_HW_LLR_LOG_NAME           "hw-llr"
#define SL_CORE_HW_AN_LOG_NAME            "hw-an"
#define SL_CORE_HW_FEC_LOG_NAME           "hw-fec"
#define SL_CORE_HW_IO_LOG_NAME            "hw-io"
#define SL_CORE_HW_SBUS_LOG_NAME          "hw-sbus"
#define SL_CORE_HW_PMI_LOG_NAME           "hw-pmi"

#define SL_CORE_TEST_FEC_LOG_NAME         "test-fec"
#define SL_CORE_TEST_AN_LOG_NAME          "test-an"
#define SL_CORE_TEST_SERDES_LOG_NAME      "test-ser"

#define SL_CORE_SERDES_LOG_NAME           "serdes"
#define SL_CORE_RESET_LOG_NAME            "reset"

#define sl_core_log_dbg(_ptr, _name, _text, ...) \
	sl_log_dbg((_ptr), SL_CORE_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_core_log_info(_ptr, _name, _text, ...) \
	sl_log_info((_ptr), SL_CORE_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_core_log_warn(_ptr, _name, _text, ...) \
	sl_log_warn((_ptr), SL_CORE_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_core_log_err(_ptr, _name, _text, ...) \
	sl_log_err((_ptr), SL_CORE_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_core_log_err_trace(_ptr, _name, _text, ...) \
	sl_log_err_trace((_ptr), SL_CORE_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)

#endif /* _SL_CORE_LOG_H_ */
