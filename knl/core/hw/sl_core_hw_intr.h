/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_INTR_H_
#define _SL_CORE_HW_INTR_H_

#include "sl_asic.h"

struct sl_core_link;

enum {
	SL_CORE_HW_INTR_LINK_UP                  = 0,
	SL_CORE_HW_INTR_LINK_HIGH_SERDES,
	SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION,
	SL_CORE_HW_INTR_LINK_LLR_STARVED,
	SL_CORE_HW_INTR_LINK_FAULT,
	SL_CORE_HW_INTR_AN_PAGE_RECV,

	SL_CORE_HW_INTR_COUNT                    /* must be last */
};

#define SL_CORE_HW_INTR_FLGS_COUNT ((int)(SS2_PORT_PML_ERR_FLG_SIZE / sizeof(u64)))

typedef void (*sl_core_hw_intr_hdlr_t)(u64 *err_flgs, int num_err_flgs, void *data);

#define SL_CORE_HW_INTR_LOG_SIZE 30

struct sl_core_hw_intr_data {
	struct sl_core_link *link;
	u32                  intr_num;
	u32                  work_num;
	char                 log[SL_CORE_HW_INTR_LOG_SIZE + 1];
};

struct sl_core_hw_intr_info {
	u64                         *flgs;
	struct sl_core_hw_intr_data  data;
	u64                          source[SL_CORE_HW_INTR_FLGS_COUNT];
};

void sl_core_hw_intr_hdlr(u64 *err_flgs, int num_err_flgs, void *data);

int  sl_core_hw_intr_hdlr_register(struct sl_core_link *link);
void sl_core_hw_intr_hdlr_unregister(struct sl_core_link *link);

void sl_core_hw_intr_flgs_check(struct sl_core_link *link, u32 which);
void sl_core_hw_intr_flgs_clr(struct sl_core_link *link, u32 which);
int  sl_core_hw_intr_flgs_enable(struct sl_core_link *link, u32 which);
int  sl_core_hw_intr_flgs_disable(struct sl_core_link *link, u32 which);

#endif /* _SL_CORE_HW_INTR_H_ */
