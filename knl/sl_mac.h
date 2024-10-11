/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MAC_H_
#define _SL_MAC_H_

#define SL_MAC_MAGIC 0x636c6c79
#define SL_MAC_VER   3
struct sl_mac {
	u32 magic;
	u32 ver;

	u8  num;
	u8  lgrp_num;
	u8  ldev_num;
};

void sl_mac_init(void);

#endif /* _SL_MAC_H_ */
