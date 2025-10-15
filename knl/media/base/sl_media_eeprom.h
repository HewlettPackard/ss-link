/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_EEPROM_H
#define _SL_MEDIA_EEPROM_H

struct sl_media_jack;
struct sl_media_attr;

void sl_media_eeprom_parse(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr);
int  sl_media_eeprom_format_get(struct sl_media_jack *media_jack, u8 *format);
u8   sl_media_eeprom_media_interface_get(struct sl_media_jack *media_jack);
void sl_media_eeprom_target_fw_ver_get(struct sl_media_jack *media_jack, char *target_fw_str, size_t target_fw_size);
bool sl_media_eeprom_is_fw_version_valid(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr);

#endif /* _SL_MEDIA_EEPROM_H */
