/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_INTR_FLGS_H_
#define _SL_CORE_HW_INTR_FLGS_H_

#include "sl_kconfig.h"
#include "sl_asic.h"

#ifdef BUILDSYS_FRAMEWORK_ROSETTA

#define SL_CORE_HW_INTR_FLGS_LINK_UP(_link_num)                                             \
	static u64 sl_core_hw_intr_flgs_link_up_##_link_num[SL_CORE_HW_INTR_FLGS_COUNT] = { \
		0ULL,                                                                       \
		0ULL,                                                                       \
		0ULL,                                                                       \
		SS2_PORT_PML_ERR_FLG_WORD3_PCS_LINK_UP_##_link_num##_SET(1),                \
	}
SL_CORE_HW_INTR_FLGS_LINK_UP(0);
SL_CORE_HW_INTR_FLGS_LINK_UP(1);
SL_CORE_HW_INTR_FLGS_LINK_UP(2);
SL_CORE_HW_INTR_FLGS_LINK_UP(3);

#define SL_CORE_HW_INTR_FLGS_LINK_HIGH_SER(_link_num)                                             \
	static u64 sl_core_hw_intr_flgs_link_high_ser_##_link_num[SL_CORE_HW_INTR_FLGS_COUNT] = { \
		0ULL,                                                                             \
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_HI_SER_##_link_num##_SET(1),                       \
		0ULL,                                                                             \
		0ULL,                                                                             \
	}
SL_CORE_HW_INTR_FLGS_LINK_HIGH_SER(0);
SL_CORE_HW_INTR_FLGS_LINK_HIGH_SER(1);
SL_CORE_HW_INTR_FLGS_LINK_HIGH_SER(2);
SL_CORE_HW_INTR_FLGS_LINK_HIGH_SER(3);

#define SL_CORE_HW_INTR_FLGS_LINK_LLR_MAX_STARVATION(_link_num)                                             \
	static u64 sl_core_hw_intr_flgs_link_llr_max_starvation_##_link_num[SL_CORE_HW_INTR_FLGS_COUNT] = { \
		0ULL,                                                                                       \
		0ULL,                                                                                       \
		SS2_PORT_PML_ERR_FLG_WORD2_LLR_MAX_STARVATION_LIMIT_##_link_num##_SET(1),                   \
		0ULL,                                                                                       \
	}
SL_CORE_HW_INTR_FLGS_LINK_LLR_MAX_STARVATION(0);
SL_CORE_HW_INTR_FLGS_LINK_LLR_MAX_STARVATION(1);
SL_CORE_HW_INTR_FLGS_LINK_LLR_MAX_STARVATION(2);
SL_CORE_HW_INTR_FLGS_LINK_LLR_MAX_STARVATION(3);

#define SL_CORE_HW_INTR_FLGS_LINK_LLR_STARVED(_link_num)                                             \
	static u64 sl_core_hw_intr_flgs_link_llr_starved_##_link_num[SL_CORE_HW_INTR_FLGS_COUNT] = { \
		0ULL,                                                                                \
		0ULL,                                                                                \
		SS2_PORT_PML_ERR_FLG_WORD2_LLR_STARVED_##_link_num##_SET(1),                         \
		0ULL,                                                                                \
	}
SL_CORE_HW_INTR_FLGS_LINK_LLR_STARVED(0);
SL_CORE_HW_INTR_FLGS_LINK_LLR_STARVED(1);
SL_CORE_HW_INTR_FLGS_LINK_LLR_STARVED(2);
SL_CORE_HW_INTR_FLGS_LINK_LLR_STARVED(3);

#define SL_CORE_HW_INTR_FLGS_LINK_FAULT(_link_num)                                             \
	static u64 sl_core_hw_intr_flgs_link_fault_##_link_num[SL_CORE_HW_INTR_FLGS_COUNT] = { \
		0ULL,                                                                          \
		SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_##_link_num##_SET(1)    |         \
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_##_link_num##_SET(1)        |         \
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_##_link_num##_SET(1)     |         \
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_##_link_num##_SET(1),              \
		0ULL,                                                                          \
		0ULL,                                                                          \
	}
SL_CORE_HW_INTR_FLGS_LINK_FAULT(0);
SL_CORE_HW_INTR_FLGS_LINK_FAULT(1);
SL_CORE_HW_INTR_FLGS_LINK_FAULT(2);
SL_CORE_HW_INTR_FLGS_LINK_FAULT(3);

#define SL_CORE_HW_INTR_FLGS_AN_PAGE_RECV(_link_num)                                             \
	static u64 sl_core_hw_intr_flgs_an_page_recv_##_link_num[SL_CORE_HW_INTR_FLGS_COUNT] = { \
		0ULL,                                                                            \
		0ULL,                                                                            \
		0ULL,                                                                            \
		SS2_PORT_PML_ERR_FLG_WORD3_AUTONEG_PAGE_RECEIVED_##_link_num##_SET(1),           \
	}
SL_CORE_HW_INTR_FLGS_AN_PAGE_RECV(0);
SL_CORE_HW_INTR_FLGS_AN_PAGE_RECV(1);
SL_CORE_HW_INTR_FLGS_AN_PAGE_RECV(2);
SL_CORE_HW_INTR_FLGS_AN_PAGE_RECV(3);

#define SL_CORE_HW_INTR_FLGS_ITEM(_num, _which)    \
	[_num] = {                                 \
		sl_core_hw_intr_flgs_##_which##_0, \
		sl_core_hw_intr_flgs_##_which##_1, \
		sl_core_hw_intr_flgs_##_which##_2, \
		sl_core_hw_intr_flgs_##_which##_3, \
	}

static u64 *sl_core_hw_intr_flgs[SL_CORE_HW_INTR_COUNT][SL_ASIC_MAX_LINKS] = {
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_UP,                 link_up),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_HIGH_SER,           link_high_ser),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION, link_llr_max_starvation),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_LLR_STARVED,        link_llr_starved),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_FAULT,              link_fault),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_AN_PAGE_RECV,            an_page_recv),
};

#else /* Cassini */

#define SL_CORE_HW_INTR_FLGS_LINK_UP(_link_num)                                        \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_link_up_##_link_num = { \
		.pcs_link_up_##_link_num = 1,                                          \
	}
SL_CORE_HW_INTR_FLGS_LINK_UP(0);

#define SL_CORE_HW_INTR_FLGS_LINK_HIGH_SER(_link_num)                                        \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_link_high_ser_##_link_num = { \
		.pcs_hi_ser_##_link_num               = 1,                                   \
	}
SL_CORE_HW_INTR_FLGS_LINK_HIGH_SER(0);

#define SL_CORE_HW_INTR_FLGS_LINK_LLR_MAX_STARVATION(_link_num)                                        \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_link_llr_max_starvation_##_link_num = { \
		.llr_max_starvation_limit_##_link_num = 1,                                             \
	}
SL_CORE_HW_INTR_FLGS_LINK_LLR_MAX_STARVATION(0);

#define SL_CORE_HW_INTR_FLGS_LINK_LLR_STARVED(_link_num)                                        \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_link_llr_starved_##_link_num = { \
		.llr_starved_##_link_num              = 1,                                      \
	}
SL_CORE_HW_INTR_FLGS_LINK_LLR_STARVED(0);

#define SL_CORE_HW_INTR_FLGS_LINK_FAULT(_link_num)                                        \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_link_fault_##_link_num = { \
		.llr_replay_at_max_##_link_num = 1,                                       \
		.pcs_link_down_##_link_num     = 1,                                       \
		.pcs_link_down_lf_##_link_num  = 1,                                       \
		.pcs_link_down_rf_##_link_num  = 1,                                       \
	}
SL_CORE_HW_INTR_FLGS_LINK_FAULT(0);

#define SL_CORE_HW_INTR_FLGS_AN_PAGE_RECV(_link_num)                                        \
	static union ss2_port_pml_err_flg sl_core_hw_intr_flgs_an_page_recv_##_link_num = { \
		.autoneg_page_received_##_link_num = 1,                                     \
	}
SL_CORE_HW_INTR_FLGS_AN_PAGE_RECV(0);

#define SL_CORE_HW_INTR_FLGS_ITEM(_num, _which)       \
	[_num] = {                                    \
		sl_core_hw_intr_flgs_##_which##_0.qw, \
	}

static u64 *sl_core_hw_intr_flgs[SL_CORE_HW_INTR_COUNT][SL_ASIC_MAX_LINKS] = {
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_UP,                 link_up),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_HIGH_SER,           link_high_ser),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION, link_llr_max_starvation),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_LLR_STARVED,        link_llr_starved),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_LINK_FAULT,              link_fault),
	SL_CORE_HW_INTR_FLGS_ITEM(SL_CORE_HW_INTR_AN_PAGE_RECV,            an_page_recv),
};

#endif /* BUILDSYS_FRAMEWORK_ROSETTA */

#endif /* _SL_CORE_HW_INTR_FLGS_H_ */
