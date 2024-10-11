/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_STR_H_
#define _SL_CORE_STR_H_

#include "sl_core_link.h"
#include "sl_core_llr.h"
#include "sl_core_mac.h"

void sl_core_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size);

const char *sl_core_link_state_str(enum sl_core_link_state link_state);

const char *sl_core_mac_state_str(enum sl_core_mac_state mac_state);

const char *sl_core_llr_state_str(enum sl_core_llr_state llr_state);
const char *sl_core_llr_flag_str(unsigned int llr_flag);

const char *sl_core_serdes_lane_state_str(u8 lane_state);
const char *sl_core_serdes_lane_encoding_str(u8 encoding);
const char *sl_core_serdes_lane_clocking_str(u8 clocking);
const char *sl_core_serdes_lane_osr_str(u8 osr);
const char *sl_core_serdes_lane_width_str(u8 width);

#endif /* _SL_CORE_STR_H_ */
