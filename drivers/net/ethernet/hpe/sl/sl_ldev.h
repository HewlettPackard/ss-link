/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_LDEV_H_
#define _SL_LDEV_H_

struct sl_ldev_attr;

#define SL_LDEV_MAGIC 0x736c636f
#define SL_LDEV_VER   1
struct sl_ldev {
	u32 magic;
	u32 ver;
	u32 size;

	u8  num;
};

int sl_ldev_attr_check(struct sl_ldev_attr *ldev_attr);

void sl_ldev_init(void);
int  sl_ldev_check(struct sl_ldev *ldev);
void sl_ldev_exit(void);

#endif /* _SL_LDEV_H_ */
