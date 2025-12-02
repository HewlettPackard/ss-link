// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>

#include "data/sl_core_data_lgrp_trace.h"

#define LOG_NAME SL_CORE_DATA_LGRP_LOG_NAME

int sl_core_data_lgrp_is_err_trace_enabled(struct sl_core_lgrp *core_lgrp, bool *err_trace_enable)
{
	spin_lock(&core_lgrp->data_lock);
	*err_trace_enable = core_lgrp->err_trace_enable;
	spin_unlock(&core_lgrp->data_lock);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "get (err_trace_enable = %s)", *err_trace_enable ? "true" : "false");

	return 0;
}

int sl_core_data_lgrp_is_warn_trace_enabled(struct sl_core_lgrp *core_lgrp, bool *warn_trace_enable)
{
	spin_lock(&core_lgrp->data_lock);
	*warn_trace_enable = core_lgrp->warn_trace_enable;
	spin_unlock(&core_lgrp->data_lock);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "get (warn_trace_enable = %s)", *warn_trace_enable ? "true" : "false");

	return 0;
}
