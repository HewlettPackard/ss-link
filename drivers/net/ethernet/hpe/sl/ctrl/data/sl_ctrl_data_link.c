// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"
#include "data/sl_core_data_link.h"

#include "data/sl_ctrl_data_link.h"

#define LOG_NAME SL_CTRL_LINK_LOG_NAME

int sl_ctrl_data_link_state_get(struct sl_ctrl_link *ctrl_link, u32 *link_state)
{
	int rtn;
	u32 core_link_state;
	u32 ctrl_link_state;

	spin_lock(&ctrl_link->data_lock);
	rtn = sl_core_data_link_state_get(sl_core_link_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
							   ctrl_link->ctrl_lgrp->num,
							   ctrl_link->num), &core_link_state);
	ctrl_link_state = ctrl_link->state;
	spin_unlock(&ctrl_link->data_lock);

	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_link, LOG_NAME,
				      "sl_core_data_link_state_get failed (rtn = %d)", rtn);
		return rtn;
	}

	*link_state = (core_link_state == SL_CORE_LINK_STATE_AN) ? SL_LINK_STATE_AN : ctrl_link_state;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"get (link_state = %u %s)",
			*link_state,
			sl_link_state_str(*link_state));

	return 0;
}
