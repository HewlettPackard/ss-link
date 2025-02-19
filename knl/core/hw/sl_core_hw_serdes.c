// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>

#include "sl_asic.h"
#include "sl_core_link.h"
#include "sl_core_lgrp.h"
#include "sl_core_ldev.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_uc_ram.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

int sl_core_hw_serdes_init(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u8  lane_num;
	u8  which;
	u32 addr;
	u32 data;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "init");

	for (lane_num = 0; lane_num < SL_ASIC_MAX_LANES; ++lane_num) {
		which = (lane_num + ((core_lgrp->num & BIT(0)) * 4));
		addr  = 0x08FFD190 | which;
		data  = 0x00000000 | (which << 24) | (which << 16);
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"init (which = %d, reg2 = 0x%08X, reg3 = 0x%08X)", which, addr, data);
		SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x2, addr);
		SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0x3, data);
	}

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_swizzles(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u8  lane_num;
	u8  which;
	u16 addr;
	u16 data;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "swizzles");

	for (lane_num = 0; lane_num < SL_ASIC_MAX_LANES; ++lane_num) {
		which = (lane_num + ((core_lgrp->num & BIT(0)) * 4));
		addr  = 0xD190 + which;
		data  = 0x0000 |
			((core_lgrp->serdes.dt.lane_info[lane_num].tx_source + ((core_lgrp->num & BIT(0)) * 4)) << 8) |
			(core_lgrp->serdes.dt.lane_info[lane_num].rx_source + ((core_lgrp->num & BIT(0)) * 4));
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"swizzles (which = %d, addr = 0x%04X, data = 0x%04X)", which, addr, data);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, addr, data, 0, 0x1F1F);
	}

	rtn = 0;
out:
	return rtn;
}

#define FW_INFO_SIGNATURE                       0x464E49
#define FW_INFO_NUM_MICROS_OFFSET               0x60
#define FW_INFO_NUM_LANES_OFFSET                0xC
#define FW_INFO_LANE_STATIC_VAR_RAM_BASE_OFFSET 0x74
#define FW_INFO_LANE_STATIC_VAR_RAM_SIZE_OFFSET 0x78
#define FW_INFO_LANE_VAR_RAM_BASE_OFFSET        0x74
#define FW_INFO_LANE_VAR_RAM_SIZE_OFFSET        0x78
#define FW_INFO_GRP_RAM_SIZE_OFFSET             0x68
#define FW_INFO_LANE_MEM_PTR_OFFSET             0x1C
#define FW_INFO_LANE_MEM_SIZE_OFFSET            0x08
#define FW_INFO_SIZE                            128

int sl_core_hw_serdes_hw_info_get(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u8  rd_buff[FW_INFO_SIZE];
	u16 data16;
	u32 data32;
	struct sl_serdes_hw_info *hw_info;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "hw info get");

	hw_info = &(core_lgrp->core_ldev->serdes.hw_info[LGRP_TO_SERDES(core_lgrp->num)]);

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD21A,  0, 12, &data16);
		hw_info->num_cores = (data16 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD10A,  0, 12, &data16);
		hw_info->num_lanes = (data16 & 0xFF);
	SL_CORE_HW_SBUS_FIELD_RD(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0xFE, 24, 0xF, &data32);
		hw_info->num_plls = (data32 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD100, 10, 10, &data16);
		hw_info->rev_id_1 = (data16 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD10E, 12, 12, &data16);
		hw_info->rev_id_2 = (data16 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD0D8,  8,  8, &data16);
		hw_info->version = (data16 & 0xFF);

	sl_core_log_dbg(core_lgrp, LOG_NAME, "rev_id_1   = 0x%02X", hw_info->rev_id_1);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "rev_id_2   = 0x%02X", hw_info->rev_id_2);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "version    = 0x%02X", hw_info->version);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "num_cores  = %u", hw_info->num_cores);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "num_lanes  = %u", hw_info->num_lanes);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "num_plls   = %u", hw_info->num_plls);

	rtn = sl_core_hw_uc_ram_rd_blk(core_lgrp, core_lgrp->serdes.dt.dev_id, 0x100, sizeof(rd_buff), rd_buff);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "uc_ram_rd_blk failed [%d]", rtn);
		goto out;
	}

	hw_info->num_micros = (*(u32 *)&rd_buff[FW_INFO_NUM_MICROS_OFFSET] & 0xF);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "num_micros = %d", hw_info->num_micros);

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_fw_info_get(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u8  rd_buff[FW_INFO_SIZE];
	struct sl_serdes_fw_info *fw_info;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw info get");

	fw_info = &(core_lgrp->core_ldev->serdes.fw_info[LGRP_TO_SERDES(core_lgrp->num)]);

	rtn = sl_core_hw_uc_ram_rd_blk(core_lgrp, core_lgrp->serdes.dt.dev_id, 0x100, sizeof(rd_buff), rd_buff);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "uc_ram_rd_blk failed [%d]", rtn);
		goto out;
	}

	fw_info->signature = (*(u32 *)rd_buff & 0xFFFFFF);
	fw_info->version   = ((*(u32 *)rd_buff >> 24) & 0xFF);

	fw_info->lane_static_var_ram_base = *(u32 *)&rd_buff[FW_INFO_LANE_STATIC_VAR_RAM_BASE_OFFSET];
	fw_info->lane_static_var_ram_size = *(u32 *)&rd_buff[FW_INFO_LANE_STATIC_VAR_RAM_SIZE_OFFSET];
	fw_info->lane_var_ram_base        = *(u32 *)&rd_buff[FW_INFO_LANE_MEM_PTR_OFFSET];
	fw_info->lane_var_ram_size        = (*(u32 *)&rd_buff[FW_INFO_LANE_MEM_SIZE_OFFSET] >> 16) & 0xFFFF;
	fw_info->grp_ram_size             = *(u32 *)&rd_buff[FW_INFO_GRP_RAM_SIZE_OFFSET] & 0xFFFF;
	fw_info->lane_count               = *(u32 *)&rd_buff[FW_INFO_NUM_LANES_OFFSET] & 0xFF;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "signature           = 0x%08X", fw_info->signature);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "version             = 0x%02X", fw_info->version);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "static var ram base = 0x%08X", fw_info->lane_static_var_ram_base);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "static var ram size = 0x%08X", fw_info->lane_static_var_ram_size);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "var ram base        = 0x%08X", fw_info->lane_var_ram_base);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "var ram size        = 0x%04X", fw_info->lane_var_ram_size);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "grp ram size        = 0x%04X", fw_info->grp_ram_size);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "lane count          = %u",     fw_info->lane_count);

	rtn = 0;
out:
	return rtn;
}

void sl_core_hw_serdes_state_set(struct sl_core_link *core_link, u8 state)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_link->serdes.data_lock, irq_flags);
	core_link->serdes.serdes_state = state;
	spin_unlock_irqrestore(&core_link->serdes.data_lock, irq_flags);
}

u8 sl_core_hw_serdes_state_get(struct sl_core_link *core_link)
{
	unsigned long irq_flags;
	u8            serdes_state;

	spin_lock_irqsave(&core_link->serdes.data_lock, irq_flags);
	serdes_state = core_link->serdes.serdes_state;
	spin_unlock_irqrestore(&core_link->serdes.data_lock, irq_flags);

	return serdes_state;
}
