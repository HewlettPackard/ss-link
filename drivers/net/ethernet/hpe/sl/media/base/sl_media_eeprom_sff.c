// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_media_jack.h"
#include "base/sl_media_eeprom_sff.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_EEPROM_SFF_LOG_NAME

#define SFF_DIAGNOSTIC_MONITORING_TYPE_OFFSET 220

int sl_media_eeprom_sff_diag_monitoring_type_get(struct sl_media_jack *media_jack,
						 u8 *diag_monitoring_type)
{
	spin_lock(&media_jack->data_lock);
	*diag_monitoring_type = media_jack->eeprom_page0[SFF_DIAGNOSTIC_MONITORING_TYPE_OFFSET];
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "sff diag monitoring type get (diag_monitoring_type = 0x%X)",
			 *diag_monitoring_type);

	return 0;
}
