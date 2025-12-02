/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_DATA_LGRP_H_
#define _SL_CORE_DATA_LGRP_H_

#include "sl_core_lgrp.h"

struct sl_hw_attr;
struct sl_lgrp_config;

int		     sl_core_data_lgrp_new(u8 ldev_num, u8 lgrp_num);
void		     sl_core_data_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id);
void		     sl_core_data_lgrp_del(u8 ldev_num, u8 lgrp_num);
struct sl_core_lgrp *sl_core_data_lgrp_get(u8 ldev_num, u8 lgrp_num);

int  sl_core_data_lgrp_link_config_check(struct sl_core_lgrp *core_lgrp);
void sl_core_data_lgrp_hw_attr_set(struct sl_core_lgrp *core_lgrp, struct sl_hw_attr *hw_attr);
void sl_core_data_lgrp_config_set(struct sl_core_lgrp *core_lgrp, struct sl_lgrp_config *lgrp_config);
int  sl_core_data_lgrp_err_trace_enable_set(struct sl_core_lgrp *core_lgrp, bool err_trace_enable);
int  sl_core_data_lgrp_warn_trace_enable_set(struct sl_core_lgrp *core_lgrp, bool warn_trace_enable);

u32  sl_core_data_lgrp_config_flags_get(struct sl_core_lgrp *core_lgrp);

#endif /* _SL_CORE_DATA_LGRP_H_ */
