/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_JACK_H_
#define _SL_MEDIA_JACK_H_

#include <linux/spinlock.h>

#include <linux/sl_media.h>

#include "sl_asic.h"
#include "sl_media_ldev.h"

#ifdef BUILDSYS_FRAMEWORK_ROSETTA
#ifndef BUILDSYS_FRAMEWORK_EMULATOR
#include <linux/hsnxcvr-api.h>
#endif /* BUILDSYS_FRAMEWORK_EMULATOR */
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */

/*
 * These states reflect whether a physical module is inserted in
 * a jack or not. And if it is inserted, can it be talked to or not.
 */
#define SL_MEDIA_JACK_CABLE_INVALID      0
#define SL_MEDIA_JACK_CABLE_REMOVED      1
#define SL_MEDIA_JACK_CABLE_INSERTED     2
#define SL_MEDIA_JACK_CABLE_GOING_ONLINE 3
#define SL_MEDIA_JACK_CABLE_ONLINE       4
#define SL_MEDIA_JACK_CABLE_ERROR        5
#define SL_MEDIA_JACK_CABLE_HIGH_TEMP    6

/*
 * GAUI interface is for AEC/AOC and GBASE interface is for PEC
 */
#define SL_MEDIA_SS1_HOST_INTERFACE_50GAUI_1_C2M     0x0A
#define SL_MEDIA_SS1_HOST_INTERFACE_100GAUI_2_C2M    0x0D
#define SL_MEDIA_SS2_HOST_INTERFACE_100GAUI_1_L_C2M  0x4C
#define SL_MEDIA_SS1_HOST_INTERFACE_200GAUI_4_C2M    0x0F
#define SL_MEDIA_SS2_HOST_INTERFACE_200GAUI_2_S_C2M  0x4D
#define SL_MEDIA_SS2_HOST_INTERFACE_400GAUI_4_S_C2M  0x4F
#define SL_MEDIA_SS1_HOST_INTERFACE_400GAUI_4_L_C2M  0x50 /* SS1 AOC/AEC default */
#define SL_MEDIA_SS2_HOST_INTERFACE_800GAUI_8_S_C2M  0x51 /* SS2 AOC/AEC default */
#define SL_MEDIA_SS1_HOST_INTERFACE_CAUI_4_C2M       0x41 /* BJ mode */
#define SL_MEDIA_SS1_HOST_INTERFACE_50GBASE_CR       0x18
#define SL_MEDIA_SS1_HOST_INTERFACE_100GBASE_CR2     0x1B
#define SL_MEDIA_SS2_HOST_INTERFACE_100GBASE_CR1     0x46
#define SL_MEDIA_SS1_HOST_INTERFACE_200GBASE_CR4     0x1C
#define SL_MEDIA_SS2_HOST_INTERFACE_200GBASE_CR2     0x47
#define SL_MEDIA_SS1_HOST_INTERFACE_400GBASE_CR8     0x1D /* SS1 PEC default */
#define SL_MEDIA_SS2_HOST_INTERFACE_400GBASE_CR4     0x48
#define SL_MEDIA_SS2_HOST_INTERFACE_800GBASE_CR8     0x49 /* SS2 PEC default */

/*
 * These states reflect the state of cable shifting for active cables.
 * For non-active cables, the cable shift state always reflects "invalid"
 */
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_INVALID             0
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_UPSHIFTED           1
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_DOWNSHIFTED         2
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_NO_CABLE     3
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_FAKE_CABLE   4
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_NO_SUPPORT   5
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_INAVLID_INFO 6
#define SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED              7

/*
 * These states reflect the state of cable shifting in the hardware.
 * These states might be different than driver states since hardware
 * state can change while driver is not loaded
 */
#define SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_INVALID     0
#define SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED   1
#define SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED 2

#define SL_MEDIA_JACK_CABLE_HIGH_TEMP_ALARM_MASK BIT(0) /* TempMonHighAlarmFlag */

/*
 * FIXME: Eventually remove this struct and get this info from cable DB
 */
struct sl_media_downshift_info {
	u32 type;
	u32 vendor;
	u8  fw_major_ver;
	u8  fw_minor_ver;
};

struct sl_media_serdes_settings {
	s16 pre1;
	s16 pre2;
	s16 pre3;
	s16 cursor;
	s16 post1;
	s16 post2;
	s16 media;
};

struct sl_media_cable_attr {
	__u32                           type;
	__u32                           vendor;
	__u32                           length_cm;
	__u32                           hpe_pn;      /* HPE part number */
	__u32                           max_speed;

	struct sl_media_serdes_settings serdes_settings;
};

struct sl_media_lgrp_cable_info {
	u8                   ldev_num;
	u8                   lgrp_num;

	struct sl_media_attr media_attr;
	struct sl_media_attr stashed_media_attr;

	int                  real_cable_status;
	int                  fake_cable_status;
};

#define SL_MEDIA_EEPROM_PAGE_SIZE 256
#define SL_MEDIA_JACK_MAGIC       0xAD91C879
struct sl_media_jack {
	u32                             magic;
	u8                              num;
	u8                              physical_num; /* number printed on jack */
	u8                              state;
	struct sl_media_ldev           *media_ldev;

	int                             cable_db_idx;
	struct sl_media_lgrp_cable_info cable_info[SL_MEDIA_MAX_LGRPS_PER_JACK];

	u8                              eeprom_page0[SL_MEDIA_EEPROM_PAGE_SIZE];
	u8                              eeprom_page1[SL_MEDIA_EEPROM_PAGE_SIZE];

	struct sl_media_serdes_settings serdes_settings;

	spinlock_t                      data_lock;
	spinlock_t                      log_lock;

	bool                            is_cable_not_supported;
	bool                            is_cable_format_invalid;
	bool                            is_ss200_cable;
	bool                            is_high_powered;
	unsigned long                   cable_power_up_wait_time_end;
	u32                             fault_cause;
	time64_t                        fault_time;
	u8                              cable_shift_state;
	u8                              appsel_no_200_gaui; /* used for downshifting */
	u8                              lane_count_200_gaui; /* used for downshifting */
	u8                              host_interface_200_gaui; /* used for downshifting */
	u8                              appsel_no_400_gaui; /* used for upshifting */
	u8                              lane_count_400_gaui; /* used for upshifting */
	u8                              host_interface_400_gaui; /* used for upshifting */

#ifdef BUILDSYS_FRAMEWORK_ROSETTA
#ifndef BUILDSYS_FRAMEWORK_EMULATOR
	void                           *hdl;
	struct xcvr_jack_data           jack_data;
	struct xcvr_status_data         status_data;
	struct xcvr_i2c_data            i2c_data;
#endif /* BUILDSYS_FRAMEWORK_EMULATOR */
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */
};

#define SL_MEDIA_FAULT_CAUSE_NONE                            0
#define SL_MEDIA_FAULT_CAUSE_EEPROM_FORMAT_INVALID           BIT(0)
#define SL_MEDIA_FAULT_CAUSE_EEPROM_VENDOR_INVALID           BIT(1)
#define SL_MEDIA_FAULT_CAUSE_EEPROM_JACK_IO                  BIT(2)
#define SL_MEDIA_FAULT_CAUSE_ONLINE_STATUS_GET               BIT(3)
#define SL_MEDIA_FAULT_CAUSE_ONLINE_TIMEDOUT                 BIT(4)
#define SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_IO                  BIT(5)
#define SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_GET                 BIT(6)
#define SL_MEDIA_FAULT_CAUSE_SERDES_SETTINGS_GET             BIT(7)
#define SL_MEDIA_FAULT_CAUSE_SCAN_STATUS_GET                 BIT(8)
#define SL_MEDIA_FAULT_CAUSE_SCAN_HDL_GET                    BIT(9)
#define SL_MEDIA_FAULT_CAUSE_SCAN_JACK_GET                   BIT(10)
#define SL_MEDIA_FAULT_CAUSE_MEDIA_ATTR_SET                  BIT(11)
#define SL_MEDIA_FAULT_CAUSE_INTR_EVENT_JACK_IO              BIT(12)
#define SL_MEDIA_FAULT_CAUSE_POWER_SET                       BIT(13)
#define SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO              BIT(14)
#define SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_LOW_POWER_SET        BIT(15)
#define SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_HIGH_POWER_SET       BIT(16)
#define SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO                BIT(17)
#define SL_MEDIA_FAULT_CAUSE_SHIFT_UP_LOW_POWER_SET          BIT(18)
#define SL_MEDIA_FAULT_CAUSE_SHIFT_UP_HIGH_POWER_SET         BIT(19)
#define SL_MEDIA_FAULT_CAUSE_SHIFT_STATE_JACK_IO             BIT(20)
#define SL_MEDIA_FAULT_CAUSE_OFFLINE                         BIT(21)
#define SL_MEDIA_FAULT_CAUSE_HIGH_TEMP_JACK_IO               BIT(22)

int                   sl_media_jack_new(struct sl_media_ldev *media_ldev, u8 jack_num);
void                  sl_media_jack_del(u8 ldev_num, u8 jack_num);
struct sl_media_jack *sl_media_jack_get(u8 ldev_num, u8 jack_num);
void                  sl_media_jack_state_set(struct sl_media_jack *media_jack, u8 state);
u8                    sl_media_jack_state_get(struct sl_media_jack *media_jack);
bool                  sl_media_jack_is_high_powered(struct sl_media_jack *media_jack);
void                  sl_media_jack_cable_shift_state_set(struct sl_media_jack *media_jack, u8 state);
u8                    sl_media_jack_cable_shift_state_get(struct sl_media_jack *media_jack);
bool                  sl_media_jack_is_cable_online(struct sl_media_jack *media_jack);
bool                  sl_media_jack_is_cable_format_invalid(struct sl_media_jack *media_jack);

u8 sl_media_jack_actv_cable_200g_host_iface_get(struct sl_media_jack *media_jack);
u8 sl_media_jack_actv_cable_200g_appsel_no_get(struct sl_media_jack *media_jack);
u8 sl_media_jack_actv_cable_200g_lane_count_get(struct sl_media_jack *media_jack);
u8 sl_media_jack_actv_cable_400g_host_iface_get(struct sl_media_jack *media_jack);
u8 sl_media_jack_actv_cable_400g_appsel_no_get(struct sl_media_jack *media_jack);
u8 sl_media_jack_actv_cable_400g_lane_count_get(struct sl_media_jack *media_jack);

int  sl_media_jack_cable_high_power_set(u8 ldev_num, u8 jack_num);
int  sl_media_jack_cable_downshift(u8 ldev_num, u8 lgrp_num, u8 link_num);
int  sl_media_jack_cable_upshift(u8 ldev_num, u8 lgrp_num, u8 link_num);
void sl_media_jack_fault_cause_set(struct sl_media_jack *media_jack, u32 fault_cause);
void sl_media_jack_fault_cause_get(struct sl_media_jack *media_jack, u32 *fault_cause,
	time64_t *fault_time);
const char *sl_media_fault_cause_str(u32 fault_cause);

bool sl_media_jack_cable_is_high_temp(struct sl_media_jack *media_jack);
int  sl_media_jack_cable_temp_get(u8 ldev_num, u8 lgrp_num, u8 *temp);

void sl_media_jack_link_led_set(u8 ldev_num, u8 lgrp_num, u32 link_state);
void sl_media_jack_headshell_led_set(u8 ldev_num, u8 lgrp_num, u8 jack_state);

#endif /* _SL_MEDIA_JACK_H_ */
