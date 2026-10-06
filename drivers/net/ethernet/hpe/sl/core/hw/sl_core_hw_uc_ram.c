// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_sbus_pmi.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_uc_ram.h"
#include "hw/sl_core_hw_serdes_addrs.h"

#define LOG_NAME SL_CORE_HW_LOG_NAME

int sl_core_hw_uc_ram_rd8(struct sl_core_lgrp *core_lgrp, u8 dev_addr,
	u8 dev_id, u8 lane_num, u32 addr, u8 *data)
{
	int  rtn;
	u16  data16;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "uc ram rd");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, lane_num, 0,
		addrs[SERDES_MICRO_A_COM_AHB_CONTROL0], 0x0000, 0x2000); /* micro_autoinc_rdaddr_en */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, lane_num, 0,
		addrs[SERDES_MICRO_A_COM_AHB_CONTROL0], 0x0000, 0x0030); /* micro_ra_rddatasize */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, lane_num, 0,
		addrs[SERDES_MICRO_A_COM_AHB_RDADDR_MSW], ((addr >> 16) & 0xFFFF), 0xFFFF); /* micro_ra_rdaddr_msw */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, lane_num, 0,
		addrs[SERDES_MICRO_A_COM_AHB_RDADDR_LSW], (addr & 0xFFFF), 0xFFFF); /* micro_ra_rdaddr_lsw */

	/* rd data */
	SL_CORE_HW_PMI_RD(core_lgrp, dev_id, lane_num, 0,
		addrs[SERDES_MICRO_A_COM_AHB_RDDATA_LSW], &data16); /* micro_ra_rddata_lsw */
	*data = (data16 & 0x00FF);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"uc ram rd8 (lane_num = %u, addr = 0x%08X, data = 0x%02X)",
		lane_num, addr, *data);

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_uc_ram_rd_blk(struct sl_core_lgrp *core_lgrp, u8 dev_addr,
	u8 dev_id, u32 start_addr, u8 rd_size, u8 *rd_buff)
{
	int  rtn;
	u8   idx;
	u16  data16;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "uc ram rd blk");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_A_COM_AHB_CONTROL0], 0x2000, 0x2000); /* micro_autoinc_rdaddr_en */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, dev_addr, dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_A_COM_AHB_CONTROL0], 0x0010, 0x0030); /* micro_ra_rddatasize */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_A_COM_AHB_RDADDR_MSW], ((start_addr >> 16) & 0xFFFF), 0xFFFF); /* micro_ra_rdaddr_msw */
	SL_CORE_HW_PMI_WR(core_lgrp, dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_A_COM_AHB_RDADDR_LSW], (start_addr & 0xFFFE), 0xFFFF); /* micro_ra_rdaddr_lsw */

	// FIXME: assume from the same block for now

	/* The micro is set up for 16-bit auto-incrementing reads starting at the even address at or below start_addr
	 * (notice 0xFFFE). Each RDDATA_LSW read returns the next little-endian 16-bit word (low byte = lower address)
	 * and advances the read address by 2.
	 *
	 * 1. Odd start - read the first word and keep only its high byte.
	 *   Example:
	 *     Read 16-bit word at 0x04: 0xBBAA
	 *     start_addr = 0x05 (odd) we keep the high byte 0xBB
	 * 2. Continue reading 16 bit words while at least 2 bytes remain in rd_size.
	 * 3. If we have a byte left read once more and keep the low byte.
	 *   Example:
	 *     Read 16-bit word at 0x06: 0xDDEE
	 *     rd_size = 1 (one byte left) we keep the low byte 0xEE
	 */
	idx = 0;
	if ((start_addr & 0x1) && rd_size) {
		SL_CORE_HW_PMI_RD(core_lgrp, dev_id, 0xFF, 0,
				  addrs[SERDES_MICRO_A_COM_AHB_RDDATA_LSW], &data16); /* micro_ra_rddata_lsw */

		rd_buff[idx] = ((data16 >> 8) & 0xFF);
		rd_size--;
		idx++;
	}

	while (rd_size >= 2) {
		SL_CORE_HW_PMI_RD(core_lgrp, dev_id, 0xFF, 0,
			addrs[SERDES_MICRO_A_COM_AHB_RDDATA_LSW], &data16); /* micro_ra_rddata_lsw */

		rd_buff[idx] = (data16 & 0xFF);
		rd_buff[idx + 1] = ((data16 >> 8) & 0xFF);
		rd_size -= 2;
		idx += 2;
	}

	if (rd_size) {
		SL_CORE_HW_PMI_RD(core_lgrp, dev_id, 0xFF, 0,
				  addrs[SERDES_MICRO_A_COM_AHB_RDDATA_LSW], &data16); /* micro_ra_rddata_lsw */

		rd_buff[idx] = (data16 & 0xFF);
	}

	rtn = 0;
out:
	return rtn;
}
