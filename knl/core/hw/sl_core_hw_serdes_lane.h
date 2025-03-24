/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_LANE_H_
#define _SL_CORE_HW_SERDES_LANE_H_

#define SL_CORE_HW_SERDES_LANE_STATE_DOWN    0
#define SL_CORE_HW_SERDES_LANE_STATE_SETUP   1
#define SL_CORE_HW_SERDES_LANE_STATE_CONFIG  2
#define SL_CORE_HW_SERDES_LANE_STATE_START   3
#define SL_CORE_HW_SERDES_LANE_STATE_CHECK   4
#define SL_CORE_HW_SERDES_LANE_STATE_STOP    5
#define SL_CORE_HW_SERDES_LANE_STATE_UP      6

#define SL_CORE_HW_SERDES_ENCODING_NRZ     2
#define SL_CORE_HW_SERDES_ENCODING_PAM4_NR 4
#define SL_CORE_HW_SERDES_ENCODING_PAM4_ER 8

#define SL_CORE_HW_SERDES_CLOCKING_82P5 BIT(0)
#define SL_CORE_HW_SERDES_CLOCKING_85   BIT(1)

#define SL_CORE_HW_SERDES_OSR_OSX1    0
#define SL_CORE_HW_SERDES_OSR_OSX2    1
#define SL_CORE_HW_SERDES_OSR_OSX4    2
#define SL_CORE_HW_SERDES_OSR_OSX42P5 33

#define SL_CORE_HW_SERDES_WIDTH_40  0
#define SL_CORE_HW_SERDES_WIDTH_80  1
#define SL_CORE_HW_SERDES_WIDTH_160 2

#define SL_CORE_HW_SERDES_SCRAMBLE_ENABLE  0
#define SL_CORE_HW_SERDES_SCRAMBLE_DISABLE 1

#define SL_CORE_HW_SERDES_DFE_DISABLE 0
#define SL_CORE_HW_SERDES_DFE_ENABLE  1

#define SL_CORE_HW_SERDES_NO_CHECK false
#define SL_CORE_HW_SERDES_CHECK    true

/* NOTES
 *   asic_lane_num   = lane indicated on the asic (0-3)
 *   serdes_lane_num = lane indicated on the serdes (0-8)
 */

struct sl_core_link;
struct sl_core_lgrp;
struct sl_core_serdes_settings;
struct sl_media_serdes_settings;

#define SL_CORE_HW_SERDES_LANE_ADDR(_addr, _lane_num, _lgrp)                                                 \
	((_lgrp)->core_ldev->serdes.fw_info[LGRP_TO_SERDES((_lgrp)->num)].lane_var_ram_base +                \
	(_addr) +                                                                                            \
	(((_lane_num) % (_lgrp)->core_ldev->serdes.fw_info[LGRP_TO_SERDES((_lgrp)->num)].lane_count) *       \
		(_lgrp)->core_ldev->serdes.fw_info[LGRP_TO_SERDES((_lgrp)->num)].lane_var_ram_size) +        \
	((_lgrp)->core_ldev->serdes.fw_info[LGRP_TO_SERDES((_lgrp)->num)].grp_ram_size * ((_lane_num) >> 1)))

#define SL_CORE_HW_SERDES_LOOPBACK_SERDES_SETTINGS_SET(_core_link)        \
	do {                                                              \
		(_core_link)->serdes.media_serdes_settings.pre3   = 0;    \
		(_core_link)->serdes.media_serdes_settings.pre2   = 0;    \
		(_core_link)->serdes.media_serdes_settings.pre1   = 0;    \
		(_core_link)->serdes.media_serdes_settings.cursor = 168;  \
		(_core_link)->serdes.media_serdes_settings.post1  = 0;    \
		(_core_link)->serdes.media_serdes_settings.post2  = 0;    \
		(_core_link)->serdes.media_serdes_settings.media  = 1;    \
	} while (0)

u8   sl_core_hw_serdes_rx_asic_lane_num_get(struct sl_core_link *core_link, u8 serdes_lane_num);
u8   sl_core_hw_serdes_tx_asic_lane_num_get(struct sl_core_link *core_link, u8 serdes_lane_num);

int  sl_core_hw_serdes_lane_pre1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre1);
int  sl_core_hw_serdes_lane_pre2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre2);
int  sl_core_hw_serdes_lane_pre3_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre3);
int  sl_core_hw_serdes_lane_cursor_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *cursor);
int  sl_core_hw_serdes_lane_post1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post1);
int  sl_core_hw_serdes_lane_post2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post2);

u16  sl_core_hw_serdes_mode(struct sl_core_lgrp *core_lgrp,
			    struct sl_core_serdes_settings *settings);
int  sl_core_hw_serdes_lane_osr_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *osr);
int  sl_core_hw_serdes_lane_encoding_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *encoding);
int  sl_core_hw_serdes_lane_width_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *width);
u16  sl_core_hw_serdes_config(struct sl_core_lgrp *core_lgrp,
			      struct sl_core_serdes_settings *core_serdes_settings,
			      struct sl_media_serdes_settings *media_serdes_settings);
int  sl_core_hw_serdes_lane_dfe_get(struct sl_core_lgrp *core_lgrp, u8 lane_num, u8 *dfe);
int  sl_core_hw_serdes_lane_scramble_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *scramble);

int  sl_core_hw_serdes_lanes_up(struct sl_core_link *core_link, bool check);
int  sl_core_hw_serdes_lane_up_tx_setup(struct sl_core_link *core_link, u8 serdes_lane_num);
int  sl_core_hw_serdes_lane_up_tx_config(struct sl_core_link *core_link, u8 serdes_lane_num);
int  sl_core_hw_serdes_lane_up_tx_start(struct sl_core_link *core_link, u8 serdes_lane_num);
int  sl_core_hw_serdes_lane_up_rx_setup(struct sl_core_link *core_link, u8 serdes_lane_num);
int  sl_core_hw_serdes_lane_up_rx_config(struct sl_core_link *core_link, u8 serdes_lane_num);
int  sl_core_hw_serdes_lane_up_rx_start(struct sl_core_link *core_link, u8 serdes_lane_num);

void sl_core_hw_serdes_lanes_down(struct sl_core_link *core_link);
void sl_core_hw_serdes_lane_down_rx_stop(struct sl_core_link *core_link, u8 serdes_lane_num);
void sl_core_hw_serdes_lane_down_tx_stop(struct sl_core_link *core_link, u8 serdes_lane_num);

int  sl_core_hw_serdes_eye_upper_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_upper);
int  sl_core_hw_serdes_eye_lower_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_lower);

void sl_core_hw_serdes_tx_lane_state_set(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u32 state);
u32  sl_core_hw_serdes_tx_lane_state_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num);
void sl_core_hw_serdes_rx_lane_state_set(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u32 state);
u32  sl_core_hw_serdes_rx_lane_state_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num);

#endif /* _SL_CORE_HW_SERDES_LANE_H_ */
