// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025-2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>

#include "sl_ctrl_lgrp.h"
#include "data/sl_ctrl_data_lgrp_trace.h"

#define LOG_NAME SL_CTRL_DATA_LGRP_LOG_NAME

int sl_ctrl_data_lgrp_is_err_trace_enabled(struct sl_ctrl_lgrp *ctrl_lgrp, bool *err_trace_enable)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctrl_lgrp->irq_trace_lock, irq_flags);
	*err_trace_enable = ctrl_lgrp->err_trace_enable;
	spin_unlock_irqrestore(&ctrl_lgrp->irq_trace_lock, irq_flags);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
			"is err trace enabled (err_trace_enable = %s)",
			*err_trace_enable ? "true" : "false");

	return 0;
}

int sl_ctrl_data_lgrp_is_warn_trace_enabled(struct sl_ctrl_lgrp *ctrl_lgrp, bool *warn_trace_enable)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctrl_lgrp->irq_trace_lock, irq_flags);
	*warn_trace_enable = ctrl_lgrp->warn_trace_enable;
	spin_unlock_irqrestore(&ctrl_lgrp->irq_trace_lock, irq_flags);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
			"is warn trace enabled (warn_trace_enable = %s)",
			*warn_trace_enable ? "true" : "false");

	return 0;
}

bool sl_ctrl_data_lgrp_is_serdes_lane_io_trace_enabled(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	unsigned long irq_flags;
	bool          serdes_lane_io_trace;

	spin_lock_irqsave(&ctrl_lgrp->irq_trace_lock, irq_flags);
	serdes_lane_io_trace = ctrl_lgrp->serdes_lane_io_trace_enable;
	spin_unlock_irqrestore(&ctrl_lgrp->irq_trace_lock, irq_flags);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
			"is serdes lane io trace enabled (serdes_lane_io_trace = %s)",
			serdes_lane_io_trace ? "enabled" : "disabled");

	return serdes_lane_io_trace;
}

