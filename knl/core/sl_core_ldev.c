// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/workqueue.h>
#include <linux/firmware.h>

#include "sl_asic.h"
#include "sl_module.h"
#include "sl_log.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "data/sl_core_data_ldev.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_serdes_fw.h"
#include "hw/sl_core_hw_serdes_core.h"
#include "hw/sl_core_hw_serdes_lane.h"

#define LOG_NAME SL_CORE_LDEV_LOG_NAME

int sl_core_ldev_new(u8 ldev_num, struct sl_accessors *accessors,
	struct sl_ops *ops, struct workqueue_struct *workqueue)
{
	return sl_core_data_ldev_new(ldev_num, accessors, ops, workqueue);
}

int sl_core_ldev_serdes_init(u8 ldev_num)
{
	int                  rtn;
	int                  lgrp_num;
	struct sl_core_lgrp *core_lgrp;
	struct sl_core_ldev *core_ldev;

	core_ldev = sl_core_ldev_get(ldev_num);
	if (!core_ldev) {
		sl_core_log_err(core_ldev, LOG_NAME,
			"ldev invalid (ldev_num = %u)", ldev_num);
		return -EIO;
	}

	sl_core_log_dbg(core_ldev, LOG_NAME, "serdes_init");

	sl_core_ldev_serdes_is_ready_set(core_ldev, false);

	rtn = request_firmware(&(core_ldev->serdes.fw), SL_HW_SERDES_FW_FILE, sl_device_get());
	if (rtn) {
		sl_core_log_err(core_ldev, LOG_NAME, "request_firmware failed [%d]", rtn);
		return -EIO;
	}

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
		core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
		if (!core_lgrp)
			continue;
		rtn = sl_core_hw_serdes_init(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_init (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
	}

	/* setup for firmware load per serdes ip */
	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; lgrp_num += 2) {
		core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
		if (!core_lgrp) {
			core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num + 1);
			if (!core_lgrp)
				continue;
		}
		rtn = sl_core_hw_serdes_fw_setup(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_fw_setup (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
	}

	/* broadcast write the firmware to all serdes */
	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; lgrp_num += 2) {
		core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
		if (!core_lgrp)
			continue;
		rtn = sl_core_hw_serdes_fw_write(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_fw_write (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
		break;
	}

	/* finish firmware load and start the micros per serdes ip */
	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; lgrp_num += 2) {
		core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
		if (!core_lgrp) {
			core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num + 1);
			if (!core_lgrp)
				continue;
		}
		rtn = sl_core_hw_serdes_fw_finish(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_fw_finish (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_fw_info_get(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_fw_info_get (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_hw_info_get(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_hw_info_get (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_core_init(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_core_init (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
		rtn = sl_core_hw_serdes_core_pll(core_lgrp, SL_CORE_HW_SERDES_CLOCKING_85);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_core_pll (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
	}

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
		core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
		if (!core_lgrp)
			continue;
		rtn = sl_core_hw_serdes_swizzles(core_lgrp);
		if (rtn) {
			sl_core_log_err(core_ldev, LOG_NAME,
				"serdes_swizzles (lgrp_num = %u) failed [%d]", lgrp_num, rtn);
			goto out;
		}
	}

	sl_core_ldev_serdes_is_ready_set(core_ldev, true);

	rtn = 0;
out:
	release_firmware(core_ldev->serdes.fw);

	return rtn;
}

void sl_core_ldev_del(u8 ldev_num)
{
	sl_core_data_ldev_del(ldev_num);
}

struct sl_core_ldev *sl_core_ldev_get(u8 ldev_num)
{
	return sl_core_data_ldev_get(ldev_num);
}

void sl_core_ldev_serdes_is_ready_set(struct sl_core_ldev *core_ldev, bool is_ready)
{
	unsigned long flags;

	sl_core_log_dbg(core_ldev, LOG_NAME,
		"set is_ready to %s", is_ready ? "true" : "false");

	spin_lock_irqsave(&core_ldev->data_lock, flags);
	core_ldev->serdes.is_ready = is_ready;
	spin_unlock_irqrestore(&core_ldev->data_lock, flags);
}

bool sl_core_ldev_serdes_is_ready(struct sl_core_ldev *core_ldev)
{
	unsigned long flags;
	bool          is_ready;

	spin_lock_irqsave(&core_ldev->data_lock, flags);
	is_ready = core_ldev->serdes.is_ready;
	spin_unlock_irqrestore(&core_ldev->data_lock, flags);

	sl_core_log_dbg(core_ldev, LOG_NAME,
		"is_ready = %s", is_ready ? "true" : "false");

	return is_ready;
}
