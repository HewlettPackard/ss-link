/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021,2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LINK_H_
#define _SL_CORE_LINK_H_

#include <linux/spinlock.h>
#include <linux/ktime.h>

#include "sl_kconfig.h"
#include <linux/sl_link.h>
#include <linux/sl_lgrp.h>
#include "sl_ctl_link_fec_priv.h"
#include "sl_core_lgrp.h"
#include "sl_core_link_an.h"
#include "sl_core_link_fec.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_serdes.h"
#include "base/sl_core_timer_link.h"
#include "base/sl_core_work_link.h"

#include "sl_media_jack.h"

struct work_struct;

#define is_flag_set(_flags, _flag) ((_flags & _flag) == _flag)

enum sl_core_info_map_bits {
	/* Headshell/Transponder */
	SL_CORE_INFO_MAP_HEADSHELL_SIGNAL = 0,
	SL_CORE_INFO_MAP_HEADSHELL_LOCK,
	SL_CORE_INFO_MAP_HEADSHELL_TIMEOUT,
	SL_CORE_INFO_MAP_HEADSHELL_OK,
	/* Serdes */
	SL_CORE_INFO_MAP_SERDES_EIDL0,
	SL_CORE_INFO_MAP_SERDES_EIDL1,
	SL_CORE_INFO_MAP_SERDES_BAD_PARAMS,
	SL_CORE_INFO_MAP_SERDES_BAD_EYES,
	SL_CORE_INFO_MAP_SERDES_OK,
	/* PCS */
	SL_CORE_INFO_MAP_PCS_BITLOCK_OK,
	SL_CORE_INFO_MAP_PCS_ALIGN_OK,
	SL_CORE_INFO_MAP_PCS_LOCAL_FAULT,
	SL_CORE_INFO_MAP_PCS_REMOTE_FAULT,
	SL_CORE_INFO_MAP_PCS_HI_SER,
	SL_CORE_INFO_MAP_PCS_LINK_DOWN,
	SL_CORE_INFO_MAP_PCS_LINK_UP,
	SL_CORE_INFO_MAP_PCS_TIMEOUT,
	SL_CORE_INFO_MAP_PCS_OK,
	/* Link Quality */
	SL_CORE_INFO_MAP_FEC_CHECK,
	SL_CORE_INFO_MAP_FEC_UCW_HIGH,
	SL_CORE_INFO_MAP_FEC_CCW_HIGH,
	SL_CORE_INFO_MAP_FEC_OK,
	/* General */
	SL_CORE_INFO_MAP_LINK_UP_TIMEOUT,
	SL_CORE_INFO_MAP_LINK_UP,
	SL_CORE_INFO_MAP_RECOVER_TIMEOUT,
	/* MAC */
	SL_CORE_INFO_MAP_MAC_RX_CONFIG,
	SL_CORE_INFO_MAP_MAC_TX_CONFIG,
	SL_CORE_INFO_MAP_MAC_RX,
	SL_CORE_INFO_MAP_MAC_TX,
	/* Autoneg */
	SL_CORE_INFO_MAP_AN_BASE_PAGE,
	SL_CORE_INFO_MAP_AN_NEXT_PAGE,
	SL_CORE_INFO_MAP_AN_ERROR,
	SL_CORE_INFO_MAP_AN_DONE,
	/* LLR */
	SL_CORE_INFO_MAP_LLR_CONFIG,
	SL_CORE_INFO_MAP_LLR_SETTING_UP,
	SL_CORE_INFO_MAP_LLR_SETUP,
	SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT,
	SL_CORE_INFO_MAP_LLR_STARTING,
	SL_CORE_INFO_MAP_LLR_RUNNING,
	SL_CORE_INFO_MAP_LLR_START_TIMEOUT,
	SL_CORE_INFO_MAP_LLR_STARVED,

	SL_CORE_INFO_MAP_NUM_BITS /* must be last */
};

typedef int (*sl_core_link_up_callback_t)(void *tag, u32 state, u32 cause, u64 info_map,
					  u32 speed, u32 fec_mode, u32 fec_type);
typedef int (*sl_core_link_fault_callback_t)(void *tag, u32 state, u32 cause, u64 info_map);
typedef int (*sl_core_link_fault_intr_hdlr_t)(u8 ldev_num, u8 lgrp_num, u8 link_num);

// FIXME: think about doing link config better
#define SL_CORE_LINK_CONFIG_MAGIC 0x736c4C43
struct sl_core_link_config {
	u32                            magic;

	u32                            link_up_timeout_ms;
	sl_core_link_fault_callback_t  fault_callback;
	sl_core_link_fault_intr_hdlr_t fault_intr_hdlr;

	u32                            fec_up_settle_wait_ms;
	u32                            fec_up_check_wait_ms;
	s32                            fec_up_ucw_limit;
	s32                            fec_up_ccw_limit;

	u32                            pause_map;
	u32                            hpe_map;

	u32                            flags;
};

struct sl_core_link_policy {
	u32 options;
};

struct sl_core_serdes_settings {
	u16 osr;
	u16 encoding;
	u16 clocking;
	u16 width;
	u16 dfe;
	u16 scramble;
};

#define SL_CORE_LINK_MAGIC 0x736c4C4E
struct sl_core_link {
	u32                              magic;
	u8                               num;

	spinlock_t                       data_lock;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link_config       config;
	struct sl_core_link_policy       policy;
	u64                              info_map;

	struct {
		spinlock_t                            data_lock;
		u32                                   state;
		u32                                   last_down_cause;
		time64_t                              last_down_time;
		void                                 *tag;
		struct {
			sl_core_link_up_callback_t    up;
		} callbacks;
		struct {
			u32                           up;
		} flags;
		bool                                  is_canceled;
		bool                                  is_timed_out;
	} link;

	struct {
		spinlock_t                            data_lock;
		u32                                   link_state;
		void                                 *tag;
		u32                                   lp_caps_get_timeout_ms;
		struct {
			sl_core_link_an_callback_t    lp_caps_get;
		} callbacks;
		struct sl_link_caps                   my_caps;
		struct sl_link_caps                   lp_caps;
		struct kmem_cache                    *lp_caps_cache;
		u32                                   lp_caps_state;
		u8                                    state;
		u8                                    page_num;
		u8                                    tx_count;
		u64                                   tx_pages[SL_CORE_LINK_AN_MAX_PAGES];
		u8                                    rx_count;
		u64                                   rx_pages[SL_CORE_LINK_AN_MAX_PAGES];
		bool                                  toggle;
		u32                                   done_work_num;
		struct sl_link_caps                   test_caps;
		bool                                  use_test_caps;
	} an;

	struct {
		struct {
			u32     speed;
			u8      pcs_mode;
			u8      tx_cdc_ready_level;
			u8      tx_en_pk_bw_limiter;
			u8      tx_gearbox_credits;
			u8      rx_restart_lock_on_bad_cws;
			u8      rx_restart_lock_on_bad_ams;
			u8      rx_active_lanes;
			u8      rs_mode;
			u16     clock_period;
			u8      mac_tx_credits;
			u8      underrun_clr;
			u8      cw_gap;
		} settings;
	} pcs;

	struct {
		u8                                       lane_map;
		struct sl_media_serdes_settings          media_serdes_settings;
		struct sl_core_serdes_settings           core_serdes_settings;
		bool                                     use_test_settings;
		struct {
			// media settings
			s16 pre1;
			s16 pre2;
			s16 pre3;
			s16 cursor;
			s16 post1;
			s16 post2;
			u16 media;
			// core settings
			u16 osr;
			u16 encoding;
			u16 clocking;
			u16 width;
			u16 dfe;
			u16 scramble;
			// options
			u32 options;
		} test_settings;
	} serdes;

	struct {
		struct sl_core_link_fec_cw_cntrs test_cntrs;
		struct sl_core_link_fec_cw_cntrs mock_cntr;
		spinlock_t                       test_lock;
		bool                             use_test_cntrs;
		struct {
			u32     mode;
			u32     type;
			u32     up_settle_wait_ms;
			u32     up_check_wait_ms;
			s32     up_ucw_limit;
			s32     up_ccw_limit;
		} settings;
		struct {
			struct sl_core_link_fec_cw_cntrs   cw;
			struct sl_core_link_fec_lane_cntrs lane;
			struct sl_core_link_fec_tail_cntrs tail;
		} up_cntrs_cache;
		struct {
			struct sl_core_link_fec_cw_cntrs   cw;
			struct sl_core_link_fec_lane_cntrs lane;
			struct sl_core_link_fec_tail_cntrs tail;
		} down_cntrs_cache;
	} fec;

	struct work_struct                            work[SL_CORE_WORK_LINK_COUNT];
	struct sl_core_timer_link_info                timers[SL_CORE_TIMER_LINK_COUNT];
	struct sl_core_hw_intr_info                   intrs[SL_CORE_HW_INTR_COUNT];
};

enum sl_core_link_state {
	SL_CORE_LINK_STATE_INVALID = 0,
	SL_CORE_LINK_STATE_UNCONFIGURED,
	SL_CORE_LINK_STATE_CONFIGURING,
	SL_CORE_LINK_STATE_CONFIGURED,
	SL_CORE_LINK_STATE_DOWN,
	SL_CORE_LINK_STATE_AN,
	SL_CORE_LINK_STATE_GOING_UP,
	SL_CORE_LINK_STATE_UP,
	SL_CORE_LINK_STATE_RECOVERING, // FIXME: need to implement this
	SL_CORE_LINK_STATE_CANCELING,
	SL_CORE_LINK_STATE_GOING_DOWN,
	SL_CORE_LINK_STATE_TIMEOUT,
};

int		     sl_core_link_new(u8 ldev_num, u8 lgrp_num, u8 link_num);
void		     sl_core_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num);
struct sl_core_link *sl_core_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_core_link_up(u8 ldev_num, u8 lgrp_num, u8 link_num,
		    sl_core_link_up_callback_t callback, void *tag);
int sl_core_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_core_link_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *link_state);
int sl_core_link_data_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_link_data *link_data);
int sl_core_info_map_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u64 *info_map);
int sl_core_link_config_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_config *link_config);
int sl_core_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_policy *link_policy);

bool sl_core_link_is_canceled_or_timed_out(struct sl_core_link *core_link);
void sl_core_link_is_canceled_set(struct sl_core_link *core_link);
void sl_core_link_is_canceled_clr(struct sl_core_link *core_link);
void sl_core_link_is_timed_out_set(struct sl_core_link *core_link);
void sl_core_link_is_timed_out_clr(struct sl_core_link *core_link);

int  sl_core_link_speed_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *speed);
int  sl_core_link_clocking_get(struct sl_core_link *core_link, u16 *clocking);

int  sl_core_link_last_down_cause_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				      u32 *down_cause, time64_t *down_time);

#endif /* _SL_CORE_LINK_H_ */