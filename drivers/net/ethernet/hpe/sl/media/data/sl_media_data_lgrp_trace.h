/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025-2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_LGRP_TRACE_H_
#define _SL_MEDIA_DATA_LGRP_TRACE_H_

#include <linux/types.h>

struct sl_media_lgrp;

int sl_media_data_lgrp_is_err_trace_enabled(struct sl_media_lgrp *media_lgrp, bool *err_trace_enable);
int sl_media_data_lgrp_is_warn_trace_enabled(struct sl_media_lgrp *media_lgrp, bool *warn_trace_enable);

#endif /* _SL_MEDIA_DATA_LGRP_TRACE_H_ */
