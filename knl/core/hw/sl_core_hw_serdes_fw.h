/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_FW_H_
#define _SL_CORE_HW_SERDES_FW_H_

struct sl_core_lgrp;

#ifdef BUILDSYS_FRAMEWORK_NIC2
#define SL_HW_SERDES_FW_FILE          "sl_fw_quad_3.04.bin"
#define SL_HW_SERDES_FW_IMAGE_CRC     0xA744 /* from the ucode file */
#define SL_HW_SERDES_FW_STACK_SIZE    0x13E4 /* from the ucode file */
#else
#define SL_HW_SERDES_FW_FILE          "sl_fw_octet_3.08.bin"
#define SL_HW_SERDES_FW_IMAGE_CRC     0x39AA /* from the ucode file */
#define SL_HW_SERDES_FW_STACK_SIZE    0x13F2 /* from the ucode file */
#endif /* BUILDSYS_FRAMEWORK_NIC2 */

int  sl_core_hw_serdes_fw_setup(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_fw_write(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_fw_finish(struct sl_core_lgrp *core_lgrp);

#endif /* _SL_CORE_HW_SERDES_FW_H_ */
