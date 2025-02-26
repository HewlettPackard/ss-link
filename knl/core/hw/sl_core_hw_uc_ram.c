// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_kconfig.h"
#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "sl_core_hw_sbus_pmi.h"
#include "sl_core_hw_pmi.h"
#include "sl_core_hw_uc_ram.h"

#define LOG_NAME SL_CORE_HW_LOG_NAME

int sl_core_hw_uc_ram_rd8(struct sl_core_lgrp *core_lgrp, u8 dev_addr,
	u8 dev_id, u8 lane_num, u32 addr, u8 *data)
{
	int rtn;
	u16 data16;

	/* disable auto increment rd addr */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, lane_num, 0, 0xD202, 0x0000, 0x2000);
	/* set rd size to 8 bits */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, lane_num, 0, 0xD202, 0x0000, 0x0030);
	/* upper 16 addr bits */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, lane_num, 0, 0xD209, ((addr >> 16) & 0xFFFF), 0, 0xFFFF);
	/* upper 16 addr bits */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, lane_num, 0, 0xD208, (addr & 0xFFFF), 0, 0xFFFF);

	/* rd data */
	SL_CORE_HW_PMI_RD(core_lgrp, dev_id, lane_num, 0, 0xD20A, 0, 0, &data16);
	*data = (data16 & 0xFF);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"uc ram rd8 (lane_num = %u, addr = 0x%08X, data = 0x%02X)",
		lane_num, addr, *data);

	rtn = 0;
out:
	return 0;
}

int sl_core_hw_uc_ram_rd_blk(struct sl_core_lgrp *core_lgrp, u8 dev_addr,
	u8 dev_id, u32 start_addr, u8 rd_size, u8 *rd_buff)
{
	int rtn;
	u8  idx;
	u16 data16;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "uc ram rd blk");

	/* enable auto increment rd addr */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, 0xFF, 0, 0xD202, 0x2000, 0x2000);
	/* select 16 bit data read size */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, 0xFF, 0, 0xD202, 0x0010, 0x0030);
	/* upper 16 addr bits */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, 0xFF, 0, 0xD209, ((start_addr >> 16) & 0xFFFF), 0, 0xFFFF);
	/* upper 16 addr bits */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, 0xFF, 0, 0xD208, (start_addr & 0xFFFE), 0, 0xFFFF);

	// FIXME: assume aligned address for now
	// FIXME: assume from the same block for now
	idx = 0;
	while (rd_size > 0) {
		SL_CORE_HW_PMI_RD(core_lgrp, dev_id, 0xFF, 0, 0xD20A, 0, 0, &data16);

		rd_buff[idx] = (data16 & 0xFF);
		rd_buff[idx + 1] = ((data16 >> 8) & 0xFF);
		rd_size -= 2;
		idx += 2;
	}

	rtn = 0;
out:
	return rtn;
}
