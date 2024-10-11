/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _SL_LGRP_H_
#define _SL_LGRP_H_

#define SL_LGRP_MAGIC 0x6c677270
#define SL_LGRP_VER   2
struct sl_lgrp {
	u32 magic;
	u32 ver;
	u32 size;

	u8  num;
	u8  ldev_num;
};

void sl_lgrp_init(void);
int  sl_lgrp_check(struct sl_lgrp *lgrp);

#endif /* _SL_LGRP_H_ */
