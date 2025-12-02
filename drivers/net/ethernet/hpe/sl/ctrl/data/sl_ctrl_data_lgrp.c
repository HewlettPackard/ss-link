// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_core_lgrp.h"

#include "data/sl_core_data_lgrp.h"
#include "data/sl_media_data_lgrp.h"

#include "sl_ctrl_data_lgrp.h"

#define LOG_NAME SL_CTRL_LGRP_LOG_NAME

int sl_ctrl_data_lgrp_err_trace_enable_set(struct sl_ctrl_lgrp *ctrl_lgrp, bool err_trace_enable)
{
	int rtn;

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "set (err_trace_enable = %s)", err_trace_enable ? "true" : "false");

	spin_lock(&ctrl_lgrp->config_lock);

	//FIXME: Maybe each layer should set this separately.
	rtn = sl_core_data_lgrp_err_trace_enable_set(sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num),
						     err_trace_enable);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME,
				      "core trace enable set failed [%d]", rtn);
		spin_unlock(&ctrl_lgrp->config_lock);
		return rtn;
	}

	rtn = sl_media_data_lgrp_err_trace_enable_set(sl_media_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num),
						      err_trace_enable);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME,
				      "media trace enable set failed [%d]", rtn);
		spin_unlock(&ctrl_lgrp->config_lock);
		return rtn;
	}

	ctrl_lgrp->err_trace_enable = err_trace_enable;

	spin_unlock(&ctrl_lgrp->config_lock);

	return 0;
}

int sl_ctrl_data_lgrp_warn_trace_set(struct sl_ctrl_lgrp *ctrl_lgrp, bool err_trace_enable)
{
	int rtn;

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "set (warn_trace_enable = %s)", err_trace_enable ? "true" : "false");

	spin_lock(&ctrl_lgrp->config_lock);

	//FIXME: Maybe each layer should set this separately.
	rtn = sl_core_data_lgrp_warn_trace_enable_set(sl_core_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num),
						      err_trace_enable);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME,
				      "core trace enable set failed [%d]", rtn);
		spin_unlock(&ctrl_lgrp->config_lock);
		return rtn;
	}

	rtn = sl_media_data_lgrp_warn_trace_enable_set(sl_media_lgrp_get(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num),
						       err_trace_enable);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME,
				      "media trace enable set failed [%d]", rtn);
		spin_unlock(&ctrl_lgrp->config_lock);
		return rtn;
	}

	ctrl_lgrp->warn_trace_enable = err_trace_enable;

	spin_unlock(&ctrl_lgrp->config_lock);

	return 0;
}

int sl_ctrl_data_lgrp_mfs_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *mfs)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*mfs = ctrl_lgrp->config.mfs;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "get (mfs = %u)", *mfs);

	return 0;
}

int sl_ctrl_data_lgrp_furcation_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *furcation)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*furcation = ctrl_lgrp->config.furcation;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "get (furcation = %u)", *furcation);

	return 0;
}

int sl_ctrl_data_lgrp_fec_mode_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *fec_mode)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*fec_mode = ctrl_lgrp->config.fec_mode;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "get (fec_mode = %u)", *fec_mode);

	return 0;
}

int sl_ctrl_data_lgrp_tech_map_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *tech_map)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*tech_map = ctrl_lgrp->config.tech_map;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "get (tech_map = 0x%X)", *tech_map);

	return 0;
}

int sl_ctrl_data_lgrp_fec_map_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *fec_map)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*fec_map = ctrl_lgrp->config.fec_map;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "get (fec_map = 0x%X)", *fec_map);

	return 0;
}

int sl_ctrl_data_lgrp_options_get(struct sl_ctrl_lgrp *ctrl_lgrp, u32 *options)
{
	spin_lock(&ctrl_lgrp->config_lock);
	*options = ctrl_lgrp->config.options;
	spin_unlock(&ctrl_lgrp->config_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "get (options = 0x%X)", *options);

	return 0;
}
