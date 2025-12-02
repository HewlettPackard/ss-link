/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_DATA_LGRP_TRACE_H_
#define _SL_CORE_DATA_LGRP_TRACE_H_

#include "sl_core_lgrp.h"

int sl_core_data_lgrp_is_err_trace_enabled(struct sl_core_lgrp *core_lgrp, bool *err_trace_enable);
int sl_core_data_lgrp_is_warn_trace_enabled(struct sl_core_lgrp *core_lgrp, bool *warn_trace_enable);

#endif /* _SL_CORE_DATA_LGRP_TRACE_H_ */
