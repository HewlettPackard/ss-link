// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_media_jack.h"
#include "base/sl_media_eeprom_cmis.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_EEPROM_CMIS_LOG_NAME

#define CMIS_FLAT_MEMORY_OFFSET            2
#define CMIS_FLAT_MEMORY_BIT               7
#define CMIS_MONITORS_ADVERTISEMENT_OFFSET 160

int sl_media_eeprom_cmis_flat_memory_get(struct sl_media_jack *media_jack, bool *flat_memory)
{
	spin_lock(&media_jack->data_lock);
	*flat_memory = media_jack->eeprom_page0[CMIS_FLAT_MEMORY_OFFSET] & BIT(CMIS_FLAT_MEMORY_BIT);
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "cmis flat memory get (flat_memory = %d)", *flat_memory);

	return 0;
}

int sl_media_eeprom_cmis_monitors_advertisement_get(struct sl_media_jack *media_jack,
						    u8 *advertisement)
{
	spin_lock(&media_jack->data_lock);
	*advertisement = media_jack->eeprom_page1[CMIS_MONITORS_ADVERTISEMENT_OFFSET];
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "cmis monitors advertisement get (advertisement = 0x%X)",
			 *advertisement);

	return 0;
}
