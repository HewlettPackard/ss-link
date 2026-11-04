/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_EEPROM_CMIS_H_
#define _SL_MEDIA_EEPROM_CMIS_H_

#include <linux/types.h>

struct sl_media_jack;

int sl_media_eeprom_cmis_flat_memory_get(struct sl_media_jack *media_jack, bool *flat_memory);
int sl_media_eeprom_cmis_monitors_advertisement_get(struct sl_media_jack *media_jack,
						    u8 *advertisement);

#endif /* _SL_MEDIA_EEPROM_CMIS_H_ */
