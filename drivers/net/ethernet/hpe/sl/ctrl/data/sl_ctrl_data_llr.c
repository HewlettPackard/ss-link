// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_data_llr.h"

#define LOG_NAME SL_CTRL_LLR_LOG_NAME

int sl_ctrl_data_llr_config_mode_get(struct sl_ctrl_llr *ctrl_llr, u32 *mode)
{
	spin_lock(&ctrl_llr->data_lock);
	*mode = ctrl_llr->config.mode;
	spin_unlock(&ctrl_llr->data_lock);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (mode = %u)\n", *mode);

	return 0;
}

int sl_ctrl_data_llr_config_setup_timeout_ms_get(struct sl_ctrl_llr *ctrl_llr, u32 *setup_timeout_ms)
{
	spin_lock(&ctrl_llr->data_lock);
	*setup_timeout_ms = ctrl_llr->config.setup_timeout_ms;
	spin_unlock(&ctrl_llr->data_lock);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (setup_timeout_ms = %u)\n", *setup_timeout_ms);

	return 0;
}

int sl_ctrl_data_llr_config_start_timeout_ms_get(struct sl_ctrl_llr *ctrl_llr, u32 *start_timeout_ms)
{
	spin_lock(&ctrl_llr->data_lock);
	*start_timeout_ms = ctrl_llr->config.start_timeout_ms;
	spin_unlock(&ctrl_llr->data_lock);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (start_timeout_ms = %u)\n", *start_timeout_ms);

	return 0;
}

int sl_ctrl_data_llr_config_link_dn_behavior_get(struct sl_ctrl_llr *ctrl_llr, u32 *link_dn_behavior)
{
	spin_lock(&ctrl_llr->data_lock);
	*link_dn_behavior = ctrl_llr->config.link_dn_behavior;
	spin_unlock(&ctrl_llr->data_lock);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (link_dn_behavior = %u)\n", *link_dn_behavior);

	return 0;
}

int sl_ctrl_data_llr_config_options_get(struct sl_ctrl_llr *ctrl_llr, u32 *options)
{
	spin_lock(&ctrl_llr->data_lock);
	*options = ctrl_llr->config.options;
	spin_unlock(&ctrl_llr->data_lock);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (options = 0x%08x)\n", *options);

	return 0;
}

