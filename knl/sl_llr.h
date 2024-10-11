/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _SL_LLR_H_
#define _SL_LLR_H_

#define SL_LLR_MAGIC 0x636c6c72
#define SL_LLR_VER   2
struct sl_llr {
	u32 magic;
	u32 ver;

	u8  num;
	u8  lgrp_num;
	u8  ldev_num;
};

void sl_llr_init(void);

#endif /* _SL_LLR_H_ */
