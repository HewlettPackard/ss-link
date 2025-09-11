/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_INTR_FLGS_H_
#define _SL_CORE_HW_INTR_FLGS_H_

#include "sl_asic.h"

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

#endif /* _SL_CORE_HW_INTR_FLGS_H_ */
