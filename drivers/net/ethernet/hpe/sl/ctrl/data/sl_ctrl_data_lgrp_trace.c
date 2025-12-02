// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>

#include "data/sl_ctrl_data_lgrp_trace.h"

#define LOG_NAME SL_CTRL_LGRP_LOG_NAME

int sl_ctrl_data_lgrp_is_err_trace_enabled(struct sl_ctrl_lgrp *ctrl_lgrp, bool *err_trace_enable)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*err_trace_enable = ctrl_lgrp->err_trace_enable;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
			"get (err_trace_enable = %s)", *err_trace_enable ? "true" : "false");

	return 0;
}

int sl_ctrl_data_lgrp_is_warn_trace_enabled(struct sl_ctrl_lgrp *ctrl_lgrp, bool *warn_trace_enable)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*warn_trace_enable = ctrl_lgrp->warn_trace_enable;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
			"get (warn_trace_enable = %s)", *warn_trace_enable ? "true" : "false");

	return 0;
}
