/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_INTR_FLGS_LLR_H_
#define _SL_CORE_HW_INTR_FLGS_LLR_H_

#include "sl_kconfig.h"
#include "sl_asic.h"

#ifdef BUILDSYS_FRAMEWORK_ROSETTA

#define SL_CORE_HW_INTR_FLGS_LLR_SETUP_UNEXP_LOOP_TIME(_llr_num)                                                 \
	static u64 sl_core_hw_intr_flgs_llr_setup_unexp_loop_time_##_llr_num[SL_CORE_HW_INTR_LLR_FLGS_COUNT] = { \
		0ULL,                                                                                            \
		0ULL,                                                                                            \
		0ULL,                                                                                            \
		SS2_PORT_PML_ERR_FLG_WORD3_LLR_UNEXP_LOOP_TIME_##_llr_num##_SET(1),                              \
	}
SL_CORE_HW_INTR_FLGS_LLR_SETUP_UNEXP_LOOP_TIME(0);
SL_CORE_HW_INTR_FLGS_LLR_SETUP_UNEXP_LOOP_TIME(1);
SL_CORE_HW_INTR_FLGS_LLR_SETUP_UNEXP_LOOP_TIME(2);
SL_CORE_HW_INTR_FLGS_LLR_SETUP_UNEXP_LOOP_TIME(3);

#define SL_CORE_HW_INTR_FLGS_LLR_SETUP_LOOP_TIME(_llr_num)                                                 \
	static u64 sl_core_hw_intr_flgs_llr_setup_loop_time_##_llr_num[SL_CORE_HW_INTR_LLR_FLGS_COUNT] = { \
		0ULL,                                                                                      \
		0ULL,                                                                                      \
		0ULL,                                                                                      \
		SS2_PORT_PML_ERR_FLG_WORD3_LLR_LOOP_TIME_##_llr_num##_SET(1),                              \
	}
SL_CORE_HW_INTR_FLGS_LLR_SETUP_LOOP_TIME(0);
SL_CORE_HW_INTR_FLGS_LLR_SETUP_LOOP_TIME(1);
SL_CORE_HW_INTR_FLGS_LLR_SETUP_LOOP_TIME(2);
SL_CORE_HW_INTR_FLGS_LLR_SETUP_LOOP_TIME(3);

#define SL_CORE_HW_INTR_FLGS_LLR_START_INIT_COMPLETE(_llr_num)                                                 \
	static u64 sl_core_hw_intr_flgs_llr_start_init_complete_##_llr_num[SL_CORE_HW_INTR_LLR_FLGS_COUNT] = { \
		0ULL,                                                                                          \
		0ULL,                                                                                          \
		0ULL,                                                                                          \
		SS2_PORT_PML_ERR_FLG_WORD3_LLR_INIT_COMPLETE_##_llr_num##_SET(1),                              \
	}
SL_CORE_HW_INTR_FLGS_LLR_START_INIT_COMPLETE(0);
SL_CORE_HW_INTR_FLGS_LLR_START_INIT_COMPLETE(1);
SL_CORE_HW_INTR_FLGS_LLR_START_INIT_COMPLETE(2);
SL_CORE_HW_INTR_FLGS_LLR_START_INIT_COMPLETE(3);

#define SL_CORE_HW_INTR_FLGS_LLR_ITEM(_num, _which)    \
	[_num] = {                                     \
		sl_core_hw_intr_flgs_llr_##_which##_0, \
		sl_core_hw_intr_flgs_llr_##_which##_1, \
		sl_core_hw_intr_flgs_llr_##_which##_2, \
		sl_core_hw_intr_flgs_llr_##_which##_3, \
	}

static u64 *sl_core_hw_intr_flgs_llr[SL_CORE_HW_INTR_LLR_COUNT][SL_ASIC_MAX_LINKS] = {
	SL_CORE_HW_INTR_FLGS_LLR_ITEM(SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME, setup_unexp_loop_time),
	SL_CORE_HW_INTR_FLGS_LLR_ITEM(SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME,       setup_loop_time),
	SL_CORE_HW_INTR_FLGS_LLR_ITEM(SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE,   start_init_complete),
};

#else /* Cassini */

#define SL_CORE_HW_INTR_FLGS_LLR_UNEXP_LOOP_TIME(_llr_num)                                              \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_llr_setup_unexp_loop_time_##_llr_num = { \
	}
SL_CORE_HW_INTR_FLGS_LLR_UNEXP_LOOP_TIME(0);

#define SL_CORE_HW_INTR_FLGS_LLR_LOOP_TIME(_llr_num)                                              \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_llr_setup_loop_time_##_llr_num = { \
		.llr_loop_time_##_llr_num = 1,                                                    \
	}
SL_CORE_HW_INTR_FLGS_LLR_LOOP_TIME(0);

#define SL_CORE_HW_INTR_FLGS_LLR_INIT_COMPLETE(_llr_num)                                              \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_llr_start_init_complete_##_llr_num = { \
		.llr_init_complete_##_llr_num = 1,                                                    \
	}
SL_CORE_HW_INTR_FLGS_LLR_INIT_COMPLETE(0);

#define SL_CORE_HW_INTR_FLGS_LLR_ITEM(_num, _which)       \
	[_num] = {                                        \
		sl_core_hw_intr_flgs_llr_##_which##_0.qw, \
	}

static u64 *sl_core_hw_intr_flgs_llr[SL_CORE_HW_INTR_LLR_COUNT][SL_ASIC_MAX_LINKS] = {
	SL_CORE_HW_INTR_FLGS_LLR_ITEM(SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME, setup_unexp_loop_time),
	SL_CORE_HW_INTR_FLGS_LLR_ITEM(SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME,       setup_loop_time),
	SL_CORE_HW_INTR_FLGS_LLR_ITEM(SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE,   start_init_complete),
};

#endif /* BUILDSYS_FRAMEWORK_ROSETTA */

#endif /* _SL_CORE_HW_INTR_FLGS_LLR_H_ */
