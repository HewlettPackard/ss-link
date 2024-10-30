/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_INTR_LLR_H_
#define _SL_CORE_HW_INTR_LLR_H_

#include "sl_asic.h"

struct sl_core_llr;

enum {
	SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME         = 0,
	SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE,

	SL_CORE_HW_INTR_LLR_COUNT                    /* must be last */
};

#define SL_CORE_HW_INTR_LLR_FLGS_COUNT ((int)(SS2_PORT_PML_ERR_FLG_SIZE / sizeof(u64)))

typedef void (*sl_core_hw_intr_llr_hdlr_t)(u64 *err_flgs, int num_err_flgs, void *data);

#define SL_CORE_HW_INTR_LLR_LOG_SIZE 30

struct sl_core_hw_intr_llr_data {
	struct sl_core_llr *core_llr;
	u32                 intr_num;
	u32                 work_num;
	char                log[SL_CORE_HW_INTR_LLR_LOG_SIZE + 1];
};

struct sl_core_hw_intr_llr_info {
	u64                             *flgs;
	struct sl_core_hw_intr_llr_data  data;
	u64                              source[SL_CORE_HW_INTR_LLR_FLGS_COUNT];
};

void sl_core_hw_intr_llr_hdlr(u64 *err_flgs, int num_err_flgs, void *data);

int  sl_core_hw_intr_llr_hdlr_register(struct sl_core_llr *core_llr);
void sl_core_hw_intr_llr_hdlr_unregister(struct sl_core_llr *core_llr);

void sl_core_hw_intr_llr_flgs_check(struct sl_core_llr *core_llr, u32 which);
void sl_core_hw_intr_llr_flgs_clr(struct sl_core_llr *core_llr, u32 which);
int  sl_core_hw_intr_llr_flgs_enable(struct sl_core_llr *core_llr, u32 which);
int  sl_core_hw_intr_llr_flgs_disable(struct sl_core_llr *core_llr, u32 which);

#endif /* _SL_CORE_HW_INTR_LLR_H_ */
