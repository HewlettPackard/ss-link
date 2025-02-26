/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_UC_RAM_H_
#define _SL_CORE_HW_UC_RAM_H_

#define SL_CORE_HW_UC_RAM_RD8(_lgrp, _dev_addr, _dev_id, _lane, _addr, _data)                            \
	do {                                                                                             \
		rtn = sl_core_hw_uc_ram_rd8((_lgrp), (_dev_addr), (_dev_id), (_lane), (_addr), (_data)); \
		if (rtn != 0)                                                                            \
			goto out;                                                                        \
	} while (0)

#define SL_CORE_HW_UC_RAM_RD_BLK(_lgrp, _dev_addr, _dev_id, _addr, _size, _buff)                            \
	do {                                                                                                \
		rtn = sl_core_hw_uc_ram_rd_blk((_lgrp), (_dev_addr), (_dev_id), (_addr), (_size), (_buff)); \
		if (rtn != 0)                                                                               \
			goto out;                                                                           \
	} while (0)

struct sl_core_lgrp;

int sl_core_hw_uc_ram_rd8(struct sl_core_lgrp *core_lgrp, u8 dev_addr,
			  u8 dev_id, u8 lane_num, u32 addr, u8 *data);
int sl_core_hw_uc_ram_rd_blk(struct sl_core_lgrp *core_lgrp, u8 dev_addr,
			     u8 dev_id, u32 start_addr, u8 rd_size, u8 *rd_buff);

#endif /* _SL_CORE_HW_UC_RAM_H_ */
