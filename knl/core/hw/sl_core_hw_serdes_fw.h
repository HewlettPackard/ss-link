/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_FW_H_
#define _SL_CORE_HW_SERDES_FW_H_

#include "sl_core_hw_serdes_firmware.h"

struct sl_core_lgrp;

int  sl_core_hw_serdes_fw_setup(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_fw_write(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_fw_finish(struct sl_core_lgrp *core_lgrp);

#endif /* _SL_CORE_HW_SERDES_FW_H_ */
