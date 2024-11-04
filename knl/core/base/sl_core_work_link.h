/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_WORK_LINK_H_
#define _SL_CORE_WORK_LINK_H_

struct sl_core_link;

enum {
	SL_CORE_WORK_LINK_AN_LP_CAPS_GET            = 0,
	SL_CORE_WORK_LINK_AN_LP_CAPS_GET_TIMEOUT,
	SL_CORE_WORK_LINK_AN_LP_CAPS_GET_DONE,
	SL_CORE_WORK_LINK_AN_UP,
	SL_CORE_WORK_LINK_AN_UP_DONE,

	SL_CORE_WORK_LINK_UP,
	SL_CORE_WORK_LINK_UP_INTR,
	SL_CORE_WORK_LINK_UP_TIMEOUT,
	SL_CORE_WORK_LINK_UP_CANCEL,
	SL_CORE_WORK_LINK_UP_CHECK,
	SL_CORE_WORK_LINK_UP_FEC_SETTLE,
	SL_CORE_WORK_LINK_UP_FEC_CHECK,

	SL_CORE_WORK_LINK_DOWN_CMD,
	SL_CORE_WORK_LINK_DOWN_FAULT,

	SL_CORE_WORK_LINK_NON_FATAL_INTR,
	SL_CORE_WORK_LINK_FAULT_INTR,

	SL_CORE_WORK_LINK_COUNT                         /* must be last */
};

void sl_core_work_link_queue(struct sl_core_link *core_link, u32 work_num);
void sl_core_work_link_flush(struct sl_core_link *core_link, u32 work_num);

#endif /* _SL_CORE_WORK_LINK_H_ */
