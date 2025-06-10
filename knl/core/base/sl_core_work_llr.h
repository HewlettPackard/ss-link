/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_WORK_LLR_H_
#define _SL_CORE_WORK_LLR_H_

enum {
	SL_CORE_WORK_LLR_SETUP                  = 0,
	SL_CORE_WORK_LLR_SETUP_TIMEOUT,
	SL_CORE_WORK_LLR_SETUP_LOOP_TIME_INTR,
	SL_CORE_WORK_LLR_SETUP_UNEXP_LOOP_TIME_INTR,
	SL_CORE_WORK_LLR_START,
	SL_CORE_WORK_LLR_START_TIMEOUT,
	SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR,

	SL_CORE_WORK_LLR_COUNT                         /* must be last */
};

#endif /* _SL_CORE_WORK_LLR_H_ */
