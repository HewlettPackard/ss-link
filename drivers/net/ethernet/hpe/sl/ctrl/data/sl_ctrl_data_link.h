/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_DATA_LINK_H_
#define _SL_CTRL_DATA_LINK_H_

#include "sl_ctrl_link.h"

int sl_ctrl_data_link_state_get(struct sl_ctrl_link *ctrl_link, u32 *link_state);

#endif /* _SL_CTRL_DATA_LINK_H_ */
