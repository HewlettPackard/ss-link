/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_DATA_LGRP_H_
#define _SL_CTRL_DATA_LGRP_H_

#include "sl_ctrl_lgrp.h"

int sl_ctrl_data_lgrp_err_trace_enable_set(struct sl_ctrl_lgrp *ctrl_lgrp, bool err_trace_enable);
int sl_ctrl_data_lgrp_warn_trace_set(struct sl_ctrl_lgrp *ctrl_lgrp, bool err_trace_enable);

int sl_ctrl_data_lgrp_mfs_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *mfs);
int sl_ctrl_data_lgrp_furcation_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *furcation);
int sl_ctrl_data_lgrp_fec_mode_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *fec_mode);
int sl_ctrl_data_lgrp_tech_map_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *tech_map);
int sl_ctrl_data_lgrp_fec_map_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *fec_map);
int sl_ctrl_data_lgrp_options_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *options);

#endif /* _SL_CTRL_DATA_LGRP_H_ */
