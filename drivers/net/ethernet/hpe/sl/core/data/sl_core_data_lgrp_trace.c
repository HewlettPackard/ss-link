// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025-2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>

#include "sl_core_lgrp.h"
#include "data/sl_core_data_lgrp_trace.h"

#define LOG_NAME SL_CORE_DATA_LGRP_LOG_NAME

int sl_core_data_lgrp_is_err_trace_enabled(struct sl_core_lgrp *core_lgrp, bool *err_trace_enable)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_lgrp->irq_trace_lock, irq_flags);
	*err_trace_enable = core_lgrp->err_trace_enable;
	spin_unlock_irqrestore(&core_lgrp->irq_trace_lock, irq_flags);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
			"is err trace enabled (err_trace_enable = %s)",
			*err_trace_enable ? "true" : "false");

	return 0;
}

int sl_core_data_lgrp_is_warn_trace_enabled(struct sl_core_lgrp *core_lgrp, bool *warn_trace_enable)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_lgrp->irq_trace_lock, irq_flags);
	*warn_trace_enable = core_lgrp->warn_trace_enable;
	spin_unlock_irqrestore(&core_lgrp->irq_trace_lock, irq_flags);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
			"is warn trace enabled (warn_trace_enable = %s)",
			*warn_trace_enable ? "true" : "false");

	return 0;
}

bool sl_core_data_lgrp_is_io_trace_enabled(struct sl_core_lgrp *core_lgrp)
{
	unsigned long irq_flags;
	bool          io_trace;

	spin_lock_irqsave(&core_lgrp->irq_trace_lock, irq_flags);
	io_trace = core_lgrp->io_trace;
	spin_unlock_irqrestore(&core_lgrp->irq_trace_lock, irq_flags);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
			"is io trace enabled (io_trace = %s)",
			io_trace ? "enabled" : "disabled");

	return io_trace;
}
