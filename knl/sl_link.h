/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _SL_LINK_H_
#define _SL_LINK_H_

#define SL_LINK_MAGIC 0x736c6c6b
#define SL_LINK_VER   1
struct sl_link {
	u32 magic;
	u32 ver;
	u32 size;

	u8  num;
	u8  lgrp_num;
	u8  ldev_num;
};

void sl_link_init(void);
int  sl_link_check(struct sl_link *link);

#endif /* _SL_LINK_H_ */
