/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SBUS_H_
#define _SL_CORE_HW_SBUS_H_

#define SL_CORE_HW_SBUS_WR(_lgrp, _dev_addr, _reg, _data)             \
	do {                                                          \
		rtn = sbus_wr((_lgrp), (_dev_addr), (_reg), (_data)); \
		if (rtn != 0)                                         \
			goto out;                                     \
	} while (0)

#define SL_CORE_HW_SBUS_FIELD_WR(_lgrp, _dev_addr, _reg, _data, _lsb, _mask)                 \
	do {                                                                                 \
		rtn = sbus_field_wr((_lgrp), (_dev_addr), (_reg), (_lsb), (_mask), (_data)); \
		if (rtn != 0)                                                                \
			goto out;                                                            \
	} while (0)

#define SL_CORE_HW_SBUS_RD(_lgrp, _dev_addr, _reg, _data)             \
	do {                                                          \
		rtn = sbus_rd((_lgrp), (_dev_addr), (_reg), (_data)); \
		if (rtn != 0)                                         \
			goto out;                                     \
	} while (0)

#define SL_CORE_HW_SBUS_FIELD_RD(_lgrp, _dev_addr, _reg, _lsb, _mask, _data)                 \
	do {                                                                                 \
		rtn = sbus_field_rd((_lgrp), (_dev_addr), (_reg), (_lsb), (_mask), (_data)); \
		if (rtn != 0)                                                                \
			goto out;                                                            \
	} while (0)

#define SL_CORE_HW_SBUS_RST(_lgrp, _dev_addr)         \
	do {                                          \
		rtn = sbus_rst((_lgrp), (_dev_addr)); \
		if (rtn != 0)                         \
			goto out;                     \
	} while (0)

#define SBUS_BROADCAST_ADDR 0xFA

struct sl_core_lgrp;

int sbus_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u32 data);
int sbus_field_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u8 lsb, u32 mask, u32 data);
int sbus_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u32 *data);
int sbus_field_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 reg, u8 lsb, u32 mask, u32 *data);
int sbus_rst(struct sl_core_lgrp *core_lgrp, u8 dev_addr);

#endif /* _SL_CORE_HW_SBUS_H_ */
