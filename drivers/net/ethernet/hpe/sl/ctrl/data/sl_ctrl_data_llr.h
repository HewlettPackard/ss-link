/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_DATA_LLR_H_
#define _SL_CTRL_DATA_LLR_H_

#include "sl_ctrl_llr.h"

int sl_ctrl_data_llr_config_mode_get(struct sl_ctrl_llr *ctrl_llr, u32 *mode);
int sl_ctrl_data_llr_config_setup_timeout_ms_get(struct sl_ctrl_llr *ctrl_llr, u32 *setup_timeout_ms);
int sl_ctrl_data_llr_config_start_timeout_ms_get(struct sl_ctrl_llr *ctrl_llr, u32 *start_timeout_ms);
int sl_ctrl_data_llr_config_link_dn_behavior_get(struct sl_ctrl_llr *ctrl_llr, u32 *link_dn_behavior);
int sl_ctrl_data_llr_config_options_get(struct sl_ctrl_llr *ctrl_llr, u32 *options);

#endif /* _SL_CTRL_DATA_LLR_H_ */
