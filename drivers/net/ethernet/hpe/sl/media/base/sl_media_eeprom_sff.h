/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_EEPROM_SFF_H_
#define _SL_MEDIA_EEPROM_SFF_H_

#include <linux/types.h>

struct sl_media_jack;

int sl_media_eeprom_sff_diag_monitoring_type_get(struct sl_media_jack *media_jack,
						 u8 *diag_monitoring_type);

#endif /* _SL_MEDIA_EEPROM_SFF_H_ */
