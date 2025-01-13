// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>
#include <linux/delay.h>
#include <linux/firmware.h>

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_module.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_uc_ram.h"
#include "hw/sl_core_hw_serdes.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

#ifdef BUILDSYS_FRAMEWORK_CASSINI
#define SL_HW_SERDES_FW_FILE          "sl_fw_quad_3.04.bin"
#define SL_HW_SERDES_FW_IMAGE_CRC     0xA744 /* from the ucode file */
#define SL_HW_SERDES_FW_STACK_SIZE    0x13E4 /* from the ucode file */
#else
#define SL_HW_SERDES_FW_FILE          "sl_fw_octet_3.08.bin"
#define SL_HW_SERDES_FW_IMAGE_CRC     0x39AA /* from the ucode file */
#define SL_HW_SERDES_FW_STACK_SIZE    0x13F2 /* from the ucode file */
#endif /* BUILDSYS_FRAMEWORK_CASSINI */

#define SL_HW_SERDES_INIT_CHECK_TRIES 50
#define SL_HW_SERDES_UC_ACTIVE_TRIES  100

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
int sl_core_hw_serdes_fw_info_get(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u8  rd_buff[FW_INFO_SIZE];
	u16 data16;
	u32 data32;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	if (core_lgrp->serdes.is_fw_info_valid)
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw info get");

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD21A,  0, 12, &data16);
	core_lgrp->serdes.hw_info.num_cores = (data16 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD10A,  0, 12, &data16);
	core_lgrp->serdes.hw_info.num_lanes = (data16 & 0xFF);
	SL_CORE_HW_SBUS_FIELD_RD(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0xFE, 24, 0xF, &data32);
	core_lgrp->serdes.hw_info.num_plls = (data32 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD100, 10, 10, &data16);
	core_lgrp->serdes.hw_info.rev_id_1 = (data16 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD10E, 12, 12, &data16);
	core_lgrp->serdes.hw_info.rev_id_2 = (data16 & 0xFF);
	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD0D8,  8,  8, &data16);
	core_lgrp->serdes.hw_info.version = (data16 & 0xFF);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"rev_id_1 = 0x%02X, rev_id_2 = 0x%02X, version = 0x%02X",
		core_lgrp->serdes.hw_info.rev_id_1, core_lgrp->serdes.hw_info.rev_id_2,
		core_lgrp->serdes.hw_info.version);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "num_cores = %u, num_lanes = %u, num_plls = %u",
		core_lgrp->serdes.hw_info.num_cores, core_lgrp->serdes.hw_info.num_lanes,
		core_lgrp->serdes.hw_info.num_plls);

	rtn = sl_core_hw_uc_ram_rd_blk(core_lgrp, core_lgrp->serdes.dt.dev_id, 0x100, sizeof(rd_buff), rd_buff);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "uc_ram_rd_blk failed [%d]", rtn);
		goto out;
	}

	core_lgrp->serdes.fw_info.signature = (*(u32 *)rd_buff & 0xFFFFFF);
	core_lgrp->serdes.fw_info.version   = ((*(u32 *)rd_buff >> 24) & 0xFF);
	if (core_lgrp->serdes.fw_info.signature != FW_INFO_SIGNATURE) {
		sl_core_log_err(core_lgrp, LOG_NAME,
			"invalid signature (expected = 0x%X, actual = 0x%X)",
			FW_INFO_SIGNATURE, core_lgrp->serdes.fw_info.signature);
		rtn = -EBADRQC;
		goto out;
	}

	core_lgrp->serdes.hw_info.num_micros               =
		(*(u32 *)&rd_buff[FW_INFO_NUM_MICROS_OFFSET] & 0xF);
	core_lgrp->serdes.fw_info.lane_static_var_ram_base =
		*(u32 *)&rd_buff[FW_INFO_LANE_STATIC_VAR_RAM_BASE_OFFSET];
	core_lgrp->serdes.fw_info.lane_static_var_ram_size =
		*(u32 *)&rd_buff[FW_INFO_LANE_STATIC_VAR_RAM_SIZE_OFFSET];
	core_lgrp->serdes.fw_info.lane_var_ram_base        =
		*(u32 *)&rd_buff[FW_INFO_LANE_MEM_PTR_OFFSET];
	core_lgrp->serdes.fw_info.lane_var_ram_size        =
		(*(u32 *)&rd_buff[FW_INFO_LANE_MEM_SIZE_OFFSET] >> 16) & 0xFFFF;
	core_lgrp->serdes.fw_info.grp_ram_size             =
		*(u32 *)&rd_buff[FW_INFO_GRP_RAM_SIZE_OFFSET] & 0xFFFF;
	core_lgrp->serdes.fw_info.lane_count               =
		*(u32 *)&rd_buff[FW_INFO_NUM_LANES_OFFSET] & 0xFF;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "signature           = 0x%08X",
		core_lgrp->serdes.fw_info.signature);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "version             = 0x%02X",
		core_lgrp->serdes.fw_info.version);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "num_micros          = %d",
		core_lgrp->serdes.hw_info.num_micros);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "static var ram base = 0x%08X",
		core_lgrp->serdes.fw_info.lane_static_var_ram_base);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "static var ram size = 0x%08X",
		core_lgrp->serdes.fw_info.lane_static_var_ram_size);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "var ram base        = 0x%08X",
		core_lgrp->serdes.fw_info.lane_var_ram_base);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "var ram size        = 0x%04X",
		core_lgrp->serdes.fw_info.lane_var_ram_size);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "grp ram size        = 0x%04X",
		core_lgrp->serdes.fw_info.grp_ram_size);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "lane count          = %u",
		core_lgrp->serdes.fw_info.lane_count);

	core_lgrp->serdes.is_fw_info_valid = true;

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_fw_image_get(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw image get \"%s\"", SL_HW_SERDES_FW_FILE);

	rtn = request_firmware(&(core_lgrp->serdes.fw), SL_HW_SERDES_FW_FILE, sl_device_get());
	if (rtn) {
		sl_core_log_err(core_lgrp, LOG_NAME, "request_firmware failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

static int sl_core_hw_serdes_fw_load_setup(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	int x;
	u16 data16;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw load setup");

	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, 0x0, 1, 0x1); /* POR_H_RSTB_SBUS */
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, 0x1, 2, 0x1); /* POR_H_RSTB_GATE */
	usleep_range(1000, 2000);
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, 0x1, 1, 0x1); /* POR_H_RSTB_SBUS */

	// FIXME: what part of this if any is actually needed?
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD23E, 0,  0, 0x0001); /* set micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD23E, 1,  0, 0x0001); /* clear micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 0,  0, 0x0001); /* disble CRC checking */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD22E, SL_HW_SERDES_FW_STACK_SIZE, 2, 0x7FFC); /* micro_core_stack_size */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD22E, 1, 15, 0x8000); /* stack enable */

	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0, 0x80000000); /* enable access to PRAM */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD200, 1, 0, 0x0001); /* turn on clocks to micro */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 1, 0, 0x0001); /* remove reset to micro */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 0, 0, 0x0001); /* set reset to micro */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 1, 0, 0x0001); /* remove reset to micro */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD227, 1, 0, 0x0001); /* enable mirco access to code RAM */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 1, 8, 0x0300); /* init code RAM to 0s */
	for (x = 0; x < SL_HW_SERDES_INIT_CHECK_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD203, 15, 15, &data16); /* check init done */
		if (data16 != 0)
			break;
	}
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 0,  8, 0x0300); /* turn off code RAM init */
	if (x >= SL_HW_SERDES_INIT_CHECK_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "fw load setup - timeout");
		rtn = -EIO;
		goto out;
	}

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 1,  2, 0x0004); /* set init CRC */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 0,  2, 0x0004); /* remove init CRC */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 1,  1, 0x0002); /* set init checksum */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 0,  1, 0x0002); /* remove init checksum */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 1,  0, 0x0001); /* enable CRC engine */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD227, 0,  1, 0x0002); /* enable writes to code RAM */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20D, 0,  2, 0xFFFC); /* set start address */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20E, 0,  0, 0xFFFF); /* set start address */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 1, 15, 0x8000); /* remove reset from PRAM */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20C, 1,  0, 0x0001); /* enable PRAM logic */

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_fw_image_write(struct sl_core_lgrp *core_lgrp)
{
	int                    rtn;
	u32                    x;
	u32                    data32;
	const struct firmware *fw;

	fw = core_lgrp->serdes.fw;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw image write (size = %lu bytes)", fw->size);

	for (x = 0; x < fw->size; x += 4) {
		data32 = (fw->data[x] |
			(fw->data[x+1] << 8) |
			(fw->data[x+2] << 16) |
			(fw->data[x+3] << 24));
		SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 10, data32);
	}

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_fw_load_finish(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	int x;
	u16 data16;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw image finish");

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD227, 1,  1, 0x0002); /* disable writes to code RAM */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 0,  0, 0x0001); /* disable CRC engine */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD22E, SL_HW_SERDES_FW_STACK_SIZE, 2, 0x7FFC); /* set stack size */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD22E, 1, 15, 0x8000); /* set stack location per micro */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20C, 0,  0, 0x0001); /* disable PRAM logic */

	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0, 0x00000000); /* disable access to RAM */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD23E, 1, 0, 0x0001); /* remove micro soft reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 1, 0, 0x0001); /* remove micro reset */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD227, 1, 0, 0x0001); /* enable micro access to code RAM */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 2, 8, 0x0300); /* init data RAM to 0s */
	for (x = 0; x < SL_HW_SERDES_INIT_CHECK_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD203, 15, 15, &data16); /* check init */
		if (data16 != 0)
			break;
	}
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 0, 8, 0x0300); /* stop data RAM init */
	if (x >= SL_HW_SERDES_INIT_CHECK_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "fw image finish - timeout");
		rtn = -EIO;
		goto out;
	}

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD21A, 0, 12, &data16);
	for (x = 0; x < data16; ++x) {
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, x, 0xD240, 1, 0, 0x0001); /* enable micro clock */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, x, 0xD241, 1, 0, 0x0001); /* remove micro reset */
	}

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD218, 0, 0, &data16); /* read CRC */
	if (data16 != SL_HW_SERDES_FW_IMAGE_CRC) {
		sl_core_log_err(core_lgrp, LOG_NAME, "fw image finish - crc check failure");
		rtn = -EIO;
		goto out;
	}

	for (x = 0; x < SL_HW_SERDES_UC_ACTIVE_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD101, 14, 15, &data16); /* read micros initialized */
		if (data16 != 1)
			continue;
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD21A, 0, 0, &data16); /* read micros active */
		if ((data16 & 0x3) != 0x3)
			continue;
		break;
	}
	if (x >= SL_HW_SERDES_UC_ACTIVE_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "fw image finish - wait uc active timeout");
		rtn = -EIO;
		goto out;
	}

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_fw_load(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	if (core_lgrp->serdes.is_fw_loaded)
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw load");

	rtn = sl_core_hw_serdes_fw_image_get(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "fw load - fw_image_get failed [%d]", rtn);
		goto out;
	}
	rtn = sl_core_hw_serdes_fw_load_setup(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "fw load - fw_load_setup failed [%d]", rtn);
		goto out;
	}
	rtn = sl_core_hw_serdes_fw_image_write(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "fw load - fw_image_write failed [%d]", rtn);
		goto out;
	}
	rtn = sl_core_hw_serdes_fw_load_finish(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "fw load - fw_load_finish failed [%d]", rtn);
		goto out;
	}

	core_lgrp->serdes.is_fw_loaded = true;

	rtn = 0;
out:
	release_firmware(core_lgrp->serdes.fw);
	return rtn;
}
