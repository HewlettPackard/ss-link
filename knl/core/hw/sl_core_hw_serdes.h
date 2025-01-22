/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_H_
#define _SL_CORE_HW_SERDES_H_

struct sl_core_link;
struct sl_core_lgrp;

#ifdef BUILDSYS_FRAMEWORK_CASSINI
#define SL_HW_SERDES_FW_FILE          "sl_fw_quad_3.04.bin"
#define SL_HW_SERDES_FW_IMAGE_CRC     0xA744 /* from the ucode file */
#define SL_HW_SERDES_FW_STACK_SIZE    0x13E4 /* from the ucode file */
#else
#define SL_HW_SERDES_FW_FILE          "sl_fw_octet_3.08.bin"
#define SL_HW_SERDES_FW_IMAGE_CRC     0x39AA /* from the ucode file */
#define SL_HW_SERDES_FW_STACK_SIZE    0x13F2 /* from the ucode file */
#endif /* BUILDSYS_FRAMEWORK_CASSINI */

int  sl_core_hw_serdes_start(struct sl_core_lgrp *core_lgrp, u32 clocking);

int  sl_core_hw_serdes_fw_load(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_fw_info_get(struct sl_core_lgrp *core_lgrp);

int  sl_core_hw_serdes_core_init(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_core_pll(struct sl_core_lgrp *core_lgrp, u32 clocking);

int  sl_core_hw_serdes_link_up_an(struct sl_core_link *core_link);
int  sl_core_hw_serdes_link_up(struct sl_core_link *core_link);

void sl_core_hw_serdes_link_down(struct sl_core_link *core_link);

void sl_core_hw_serdes_state_set(struct sl_core_link *core_link, u8 state);
u8   sl_core_hw_serdes_state_get(struct sl_core_link *core_link);

#endif /* _SL_CORE_HW_SERDES_H_ */
