/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021,2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LINK_H_
#define _SL_CORE_LINK_H_

#include <linux/spinlock.h>
#include <linux/ktime.h>

#include <linux/sl_link.h>
#include <linux/sl_lgrp.h>

#include "sl_kconfig.h"
#include "sl_ctl_link_fec_priv.h"
#include "sl_core_lgrp.h"
#include "sl_core_link_an.h"
#include "sl_core_link_fec.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_reset.h"
#include "base/sl_core_timer_link.h"
#include "base/sl_core_work_link.h"

#include "sl_media_jack.h"

struct work_struct;

#define is_flag_set(_flags, _flag) ((_flags & _flag) == _flag)

#define SL_LINK_DOWN_CAUSE_SERDES_PLL_MAP (            \
		SL_LINK_DOWN_CAUSE_SERDES_PLL        | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_SERDES_CONFIG_MAP (         \
		SL_LINK_DOWN_CAUSE_SERDES_CONFIG     | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_SERDES_SIGNAL_MAP (         \
		SL_LINK_DOWN_CAUSE_SERDES_SIGNAL     | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_SERDES_QUALITY_MAP (        \
		SL_LINK_DOWN_CAUSE_SERDES_QUALITY    | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_INTR_ENABLE_MAP (           \
		SL_LINK_DOWN_CAUSE_INTR_ENABLE       | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_INTR_REGISTER_MAP (         \
		SL_LINK_DOWN_CAUSE_INTR_REGISTER     | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH_MAP (       \
		SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH   | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_CONFIG_MAP (                \
		SL_LINK_DOWN_CAUSE_CONFIG            | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_AUTONEG_MAP (               \
		SL_LINK_DOWN_CAUSE_AUTONEG           | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_TIMEOUT_MAP (               \
		SL_LINK_DOWN_CAUSE_TIMEOUT           | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE_MAP (     \
		SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_MEDIA_ERROR_MAP (           \
		SL_LINK_DOWN_CAUSE_MEDIA_ERROR       | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_DOWNSHIFT_MAP (             \
		SL_LINK_DOWN_CAUSE_DOWNSHIFT         | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_UPSHIFT_MAP (               \
		SL_LINK_DOWN_CAUSE_UPSHIFT           | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG_MAP (        \
		SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG    | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG_MAP (        \
		SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG    | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_PCS_FAULT_MAP (             \
		SL_LINK_DOWN_CAUSE_PCS_FAULT         | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)
#define SL_LINK_DOWN_CAUSE_UCW_UP_CHECK_MAP (          \
		SL_LINK_DOWN_CAUSE_UCW               | \
		SL_LINK_DOWN_RETRYABLE               | \
		SL_LINK_DOWN_ORIGIN_LINK_UP)
#define SL_LINK_DOWN_CAUSE_CCW_UP_CHECK_MAP (          \
		SL_LINK_DOWN_CAUSE_CCW               | \
		SL_LINK_DOWN_RETRYABLE               | \
		SL_LINK_DOWN_ORIGIN_LINK_UP)
#define SL_LINK_DOWN_CAUSE_LF_MAP (                    \
		SL_LINK_DOWN_CAUSE_LF                | \
		SL_LINK_DOWN_RETRYABLE               | \
		SL_LINK_DOWN_ORIGIN_ASYNC)
#define SL_LINK_DOWN_CAUSE_RF_MAP (                    \
		SL_LINK_DOWN_CAUSE_RF                | \
		SL_LINK_DOWN_RETRYABLE               | \
		SL_LINK_DOWN_ORIGIN_ASYNC)
#define SL_LINK_DOWN_CAUSE_DOWN_MAP (                  \
		SL_LINK_DOWN_CAUSE_DOWN              | \
		SL_LINK_DOWN_RETRYABLE               | \
		SL_LINK_DOWN_ORIGIN_ASYNC)
#define SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX_MAP (        \
		SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX    | \
		SL_LINK_DOWN_RETRYABLE               | \
		SL_LINK_DOWN_ORIGIN_ASYNC)
#define SL_LINK_DOWN_CAUSE_CANCELED_MAP (              \
		SL_LINK_DOWN_CAUSE_CANCELED          | \
		SL_LINK_DOWN_ORIGIN_LINK_UP          | \
		SL_LINK_DOWN_RETRYABLE)

#define SL_LINK_DEGRADE_STATE_INVALID   0
#define SL_LINK_DEGRADE_STATE_ENABLED   1
#define SL_LINK_DEGRADE_STATE_DISABLED  2
#define SL_LINK_DEGRADE_STATE_FAILED    3

enum sl_core_info_map_bits {
	/* Media */
	SL_CORE_INFO_MAP_MEDIA_CHECK,        /* Indicates the link up process is checking media */
	SL_CORE_INFO_MAP_MEDIA_OK,           /* Indicates the link up media check completed successfully */

	/* SerDes */
	SL_CORE_INFO_MAP_SERDES_START,       /* Indicates the SerDes is starting */
	SL_CORE_INFO_MAP_SERDES_CHECK,       /* Indicates the SerDes is being checked */
	SL_CORE_INFO_MAP_SERDES_OK,          /* Indicates the SerDes check is complete */

	/* PCS */
	SL_CORE_INFO_MAP_PCS_LOCAL_FAULT,    /* Indicates the link is going down from a local fault interrupt */
	SL_CORE_INFO_MAP_PCS_REMOTE_FAULT,   /* Indicates the link is going down from a remote fault interrupt */
	SL_CORE_INFO_MAP_PCS_LINK_DOWN,      /* Indicates the link is going down from a link down interrupt */
	SL_CORE_INFO_MAP_PCS_CHECK,          /* Indicates the PCS is being checked */
	SL_CORE_INFO_MAP_PCS_OK,             /* Indicates the PCS check is complete */

	/* Link Quality */
	SL_CORE_INFO_MAP_FEC_CHECK,          /* Indicates link up process is checking FEC */
	SL_CORE_INFO_MAP_FEC_OK,             /* Indicates link up FEC check completed successfully */
	SL_CORE_INFO_MAP_LINK_DEGRADED,      /* Indicates the link degradation process started */

	/* Link Forward Progress */
	SL_CORE_INFO_MAP_LINK_DOWN,          /* Indicates the link has been commanded down */
	SL_CORE_INFO_MAP_LINK_UP_CANCEL,     /* Indicates the link up process has been canceled */
	SL_CORE_INFO_MAP_LINK_UP_TIMEOUT,    /* Indicates the link up process has timed out */
	SL_CORE_INFO_MAP_LINK_UP_FAIL,       /* Indicates the link up process has failed */
	SL_CORE_INFO_MAP_LINK_UP,            /* Indicates the link up process completed successfully */

	/* MAC */
	SL_CORE_INFO_MAP_MAC_RX_CONFIG,      /* Indicates MAC RX configuration is in progress */
	SL_CORE_INFO_MAP_MAC_TX_CONFIG,      /* Indicates MAC TX configuration is in progress */
	SL_CORE_INFO_MAP_MAC_RX,             /* Indicates MAC RX is operational */
	SL_CORE_INFO_MAP_MAC_TX,             /* Indicates MAC TX is operational */

	/* Autoneg */
	SL_CORE_INFO_MAP_AN_BASE_PAGE,       /* Indicates the base page of autonegotiation is being sent */
	SL_CORE_INFO_MAP_AN_NEXT_PAGE,       /* Indicates the next page of autonegotiation is being sent */
	SL_CORE_INFO_MAP_AN_ERROR,           /* Indicates an error occurred during autonegotiation */
	SL_CORE_INFO_MAP_AN_DONE,            /* Indicates autonegotiation completed successfully */

	/* LLR */
	SL_CORE_INFO_MAP_LLR_CONFIG,         /* Indicates LLR is being configured */
	SL_CORE_INFO_MAP_LLR_SETTING_UP,     /* Indicates LLR is setting up */
	SL_CORE_INFO_MAP_LLR_SETUP,          /* Indicates LLR setup completed successfully */
	SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT,  /* Indicates LLR setup has timed out */
	SL_CORE_INFO_MAP_LLR_STARTING,       /* Indicates LLR is starting */
	SL_CORE_INFO_MAP_LLR_RUNNING,        /* Indicates LLR is running */
	SL_CORE_INFO_MAP_LLR_START_TIMEOUT,  /* Indicates LLR start process timed out */

	SL_CORE_INFO_MAP_HIGH_SER,           /* Indicates High Symbol Error ratio work started */
	SL_CORE_INFO_MAP_LLR_MAX_STARVATION, /* Inciates LLR Max Starvation work started */
	SL_CORE_INFO_MAP_LLR_STARVED,        /* Indicates LLR starved work started */
	SL_CORE_INFO_MAP_LLR_REPLAY_MAX,     /* Indicates link is going down from LLR replay at max */

	SL_CORE_INFO_MAP_NUM_BITS            /* must be last */
};

struct sl_core_link_up_info {
	u32 state;
	u64 cause_map;
	u64 info_map;
	u32 speed;
	u32 fec_mode;
	u32 fec_type;
};

typedef int (*sl_core_link_up_callback_t)(void *tag, struct sl_core_link_up_info *core_link_up_info);
typedef int (*sl_core_link_down_callback_t)(void *tag, u32 state, u64 cause_map, u64 info_map);
typedef int (*sl_core_link_fault_callback_t)(void *tag, u32 state, u64 cause_map, u64 info_map);
typedef int (*sl_core_link_fault_intr_hdlr_t)(u8 ldev_num, u8 lgrp_num, u8 link_num);

// FIXME: think about doing link config better
#define SL_CORE_LINK_CONFIG_MAGIC 0x736c4C43
struct sl_core_link_config {
	u32                            magic;

	u32                            link_up_timeout_ms;
	sl_core_link_fault_callback_t  fault_callback;
	sl_core_link_fault_intr_hdlr_t fault_intr_hdlr;

	s32                            fec_up_settle_wait_ms;
	s32                            fec_up_check_wait_ms;
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
	int                              degrade_state;
	struct sl_link_degrade_info      degrade_info;
	u64                              info_map;

	spinlock_t                       irq_data_lock;

	struct {
		spinlock_t                            data_lock;
		u32                                   state;
		u64                                   last_up_fail_cause_map;
		time64_t                              last_up_fail_time;
		bool                                  is_last_down_new;
		u64                                   last_down_cause_map;
		time64_t                              last_down_time;
		struct {
			void                         *up;
			void                         *down;
		} tags;
		struct {
			sl_core_link_up_callback_t    up;
			sl_core_link_down_callback_t  down;
		} callbacks;
		struct {
			u32                           up;
		} flags;
		bool                                  is_ccw_warn_limit_crossed;
		time64_t                              last_ccw_warn_limit_crossed_time;
		bool                                  is_ucw_warn_limit_crossed;
		time64_t                              last_ucw_warn_limit_crossed_time;
	} link;

	struct {
		spinlock_t                            data_lock;
		u32                                   link_state;
		void                                 *tag;
		struct {
			sl_core_link_an_callback_t    lp_caps_get;
		} callbacks;
		struct sl_link_caps                   my_caps;
		struct sl_link_caps                   lp_caps;
		u32                                   lp_caps_state;
		u8                                    state;
		u32                                   fail_cause;
		time64_t                              fail_time;
		u8                                    page_num;
		u8                                    tx_count;
		u64                                   tx_pages[SL_CORE_LINK_AN_MAX_PAGES];
		u8                                    rx_count;
		u64                                   rx_pages[SL_CORE_LINK_AN_MAX_PAGES];
		bool                                  toggle;
		u32                                   done_work_num;
		struct sl_link_caps                   test_caps;
		bool                                  use_test_caps;
		u16                                   restart_sleep_ms;
		atomic_t                              retry_count;
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
		u8                               lane_map;
		struct sl_media_serdes_settings  media_serdes_settings;
		struct sl_core_serdes_settings   core_serdes_settings;
		bool                             use_test_settings;
		spinlock_t                       data_lock;
		u8                               serdes_state;
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
int sl_core_link_up_fail(struct sl_core_link *core_link);
int sl_core_link_cancel(u8 ldev_num, u8 lgrp_num, u8 link_num,
		      sl_core_link_down_callback_t callback, void *tag);
int sl_core_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num,
		      sl_core_link_down_callback_t callback, void *tag, u64 down_cause_map);
int sl_core_link_reset(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_core_link_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *link_state);
u64 sl_core_link_info_map_get(u8 ldev_num, u8 lgrp_num, u8 link_num);
int sl_core_info_map_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u64 *info_map);
int sl_core_link_config_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_config *link_config);
int sl_core_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_policy *link_policy);
int sl_core_link_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_link_caps *link_caps);

bool sl_core_link_config_is_enable_ald_set(struct sl_core_link *core_link);
bool sl_core_link_is_degrade_state_enabled(struct sl_core_link *core_link);

bool sl_core_link_is_canceled_or_timed_out(struct sl_core_link *core_link);

int  sl_core_link_speed_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *speed);
int  sl_core_link_clocking_get(struct sl_core_link *core_link, u16 *clocking);

void sl_core_link_last_down_cause_map_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
					       u64 down_cause_map);
void sl_core_link_last_down_cause_map_info_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
					       u64 *down_cause_map, time64_t *down_time);
void sl_core_link_last_up_fail_cause_map_set(u8 ldev_num, u8 lgrp_num, u8 link_num, u64 up_fail_cause_map);

void sl_core_link_ucw_warn_limit_crossed_get(u8 ldev_num, u8 lgrp_num, u8 link_num, bool *is_limit_crossed,
					     time64_t *limit_crossed_time);
void sl_core_link_ucw_warn_limit_crossed_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool is_limit_crossed);
void sl_core_link_ccw_warn_limit_crossed_get(u8 ldev_num, u8 lgrp_num, u8 link_num, bool *is_limit_crossed,
					     time64_t *limit_crossed_time);
void sl_core_link_ccw_warn_limit_crossed_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool is_limit_crossed);

bool sl_core_link_policy_is_keep_serdes_up_set(struct sl_core_link *core_link);
bool sl_core_link_policy_is_use_unsupported_cable_set(struct sl_core_link *core_link);
bool sl_core_link_policy_is_ignore_media_errors_set(struct sl_core_link *core_link);

struct sl_core_link_up_info *sl_core_link_up_info_get(struct sl_core_link *core_link,
	struct sl_core_link_up_info *link_up_info);

#endif /* _SL_CORE_LINK_H_ */
