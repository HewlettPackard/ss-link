// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/umh.h>
#include <linux/delay.h>
#include <linux/firmware.h>

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_sbus_pmi.h"
#include "hw/sl_core_hw_serdes_fw.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

#define SL_HW_SERDES_INIT_CHECK_TRIES 50
int sl_core_hw_serdes_fw_setup(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	int x;
	u16 data16;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw setup");

	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, 0x0, 1, 0x1); /* POR_H_RSTB_SBUS */
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, 0x1, 2, 0x1); /* POR_H_RSTB_GATE */
	usleep_range(1000, 2000);
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, 0x1, 1, 0x1); /* POR_H_RSTB_SBUS */

	for (x = 0; x < SL_SERDES_NUM_MICROS; ++x) {
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, x, 0xD240, 0, 0, 0x0001); /* disable micro clock */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, x, 0xD241, 0, 0, 0x0001); /* set micro reset */
	}

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD227, 0x0001, 0x0001); /* enable writes to code RAM */

	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0, 0x80000000); /* enable access to PRAM */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD200, 1, 0, 0x0001); /* enable micro master clocks */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 0, 0, 0x0001); /* set micro master reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 1, 0, 0x0001); /* remove micro master reset */

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD227, 0x0000, 0x0002); /* disable micro access to code RAM */

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 0x0100, 0x0300); /* init code RAM */
	for (x = 0; x < SL_HW_SERDES_INIT_CHECK_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD203, 15, 15, &data16); /* check init done */
		if (data16 != 0)
			break;
	}
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 0x0000, 0x0300); /* disable init */
	if (x >= SL_HW_SERDES_INIT_CHECK_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "fw setup RAM init timeout");
		rtn = -EIO;
		goto out;
	}

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 0x0200, 0x0300); /* init data RAM */
	for (x = 0; x < SL_HW_SERDES_INIT_CHECK_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD203, 15, 15, &data16); /* check init done */
		sl_core_log_dbg(core_lgrp, LOG_NAME, "code RAM init D203 = 0x%X", data16);
		if (data16 != 0)
			break;
	}
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD202, 0x0000, 0x0300); /* disable init */
	if (x >= SL_HW_SERDES_INIT_CHECK_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "code RAM init timeout");
		rtn = -EIO;
		goto out;
	}

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 1,  2, 0x0004); /* set init CRC */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 0,  2, 0x0004); /* remove init CRC */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 1,  1, 0x0002); /* set init checksum */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 0,  1, 0x0002); /* remove init checksum */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 1,  0, 0x0001); /* enable CRC engine */

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20D, 0x0000, 0xFFFC); /* start address */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20E, 0x0000, 0xFFFF); /* start address */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD201, 1, 15, 0x8000); /* remove reset from PRAM */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20C, 0x0001, 0x0001); /* enable PRAM interface */

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_fw_write(struct sl_core_lgrp *core_lgrp)
{
	int                    rtn;
	u32                    x;
	u32                    data32;
	const struct firmware *fw;

	fw = core_lgrp->core_ldev->serdes.fw;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw write (size = %lu bytes)", fw->size);

	for (x = 0; x < fw->size; x += 4) {
		data32 = (fw->data[x] |
			(fw->data[x+1] << 8) |
			(fw->data[x+2] << 16) |
			(fw->data[x+3] << 24));
		SL_CORE_HW_SBUS_WR(core_lgrp, 0xFA, 10, data32);
	}

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_fw_finish(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u16 data16;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "fw finish");

	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD227, 0x0002, 0x0002); /* disable writes to code RAM */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD217, 0,  0, 0x0001); /* disable CRC engine */
	SL_CORE_HW_SBUS_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD20C, 0x0000, 0x0001); /* disable PRAM interface */

	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0, 0x00000000); /* disable access to PRAM */

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD218, 0, 0, &data16); /* read CRC */
	if (data16 != SL_HW_SERDES_FW_IMAGE_CRC) {
		sl_core_log_err(core_lgrp, LOG_NAME,
			"fw finish crc check failure (crc = 0x%04X, expected = 0x%04X)",
			data16, SL_HW_SERDES_FW_IMAGE_CRC);
		rtn = -EIO;
		goto out;
	}

	rtn = 0;
out:
	return rtn;
}
