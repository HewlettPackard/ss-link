// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>

#include "data/sl_media_data_lgrp_trace.h"

#define LOG_NAME SL_MEDIA_DATA_LGRP_LOG_NAME

int sl_media_data_lgrp_is_err_trace_enabled(struct sl_media_lgrp *media_lgrp, bool *err_trace_enable)
{
	spin_lock(&media_lgrp->data_lock);
	*err_trace_enable = media_lgrp->err_trace_enable;
	spin_unlock(&media_lgrp->data_lock);

	sl_media_log_dbg(media_lgrp, LOG_NAME, "get (err_trace_enable = %s)", *err_trace_enable ? "true" : "false");

	return 0;
}

int sl_media_data_lgrp_is_warn_trace_enabled(struct sl_media_lgrp *media_lgrp, bool *warn_trace_enable)
{
	spin_lock(&media_lgrp->data_lock);
	*warn_trace_enable = media_lgrp->warn_trace_enable;
	spin_unlock(&media_lgrp->data_lock);

	sl_media_log_dbg(media_lgrp, LOG_NAME, "get (warn_trace_enable = %s)", *warn_trace_enable ? "true" : "false");

	return 0;
}
