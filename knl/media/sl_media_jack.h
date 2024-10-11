/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_JACK_H_
#define _SL_MEDIA_JACK_H_

#include <linux/spinlock.h>
#include "sl_asic.h"
#include "sl_media_ldev.h"
#include "base/sl_media_log.h"
#include "uapi/sl_media.h"

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

struct sl_media_serdes_settings {
	s16 pre1;
	s16 pre2;
	s16 pre3;
	s16 cursor;
	s16 post1;
	s16 post2;
	u16 media;
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
	bool                            is_high_powered;
	unsigned long                   cable_power_up_wait_time_end;

#ifdef BUILDSYS_FRAMEWORK_ROSETTA
#ifndef BUILDSYS_FRAMEWORK_EMULATOR
	void                           *hdl;
	struct xcvr_jack_data           jack_data;
	struct xcvr_status_data         status_data;
	struct xcvr_i2c_data            i2c_data;
#endif /* BUILDSYS_FRAMEWORK_EMULATOR */
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */
};

int                   sl_media_jack_new(struct sl_media_ldev *media_ldev, u8 jack_num);
void                  sl_media_jack_del(u8 ldev_num, u8 jack_num);
struct sl_media_jack *sl_media_jack_get(u8 ldev_num, u8 jack_num);
u8                    sl_media_jack_state_get(struct sl_media_jack *media_jack);
bool                  sl_media_jack_is_cable_online(struct sl_media_jack *media_jack);
bool                  sl_media_jack_is_cable_format_invalid(struct sl_media_jack *media_jack);

int  sl_media_jack_cable_high_power_set(u8 ldev_num, u8 jack_num);

#endif /* _SL_MEDIA_JACK_H_ */
