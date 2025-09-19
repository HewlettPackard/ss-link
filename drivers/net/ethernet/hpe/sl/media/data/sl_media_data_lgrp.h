/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_LGRP_H_
#define _SL_MEDIA_DATA_LGRP_H_

struct sl_media_lgrp;

int                   sl_media_data_lgrp_new(u8 ldev_num, u8 lgrp_num);
void                  sl_media_data_lgrp_del(u8 ldev_num, u8 lgrp_num);
struct sl_media_lgrp *sl_media_data_lgrp_get(u8 ldev_num, u8 lgrp_num);

void                  sl_media_data_lgrp_connect_id_set(struct sl_media_lgrp *lgrp, const char *connect_id);

#endif
