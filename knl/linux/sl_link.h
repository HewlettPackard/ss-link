/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_LINK_H_
#define _LINUX_SL_LINK_H_

#include <linux/bitops.h>
#include <linux/kobject.h>

struct sl_lgrp;
struct sl_link;

#define SL_LINK_INFO_STRLEN 128

enum sl_link_state {
	SL_LINK_STATE_INVALID,
	SL_LINK_STATE_DOWN,
	SL_LINK_STATE_AN,
	SL_LINK_STATE_STARTING,
	SL_LINK_STATE_UP,
	SL_LINK_STATE_STOPPING,
};

#define SL_LINK_CAPS_MAGIC 0x6D676774
#define SL_LINK_CAPS_VER   1
struct sl_link_caps {
	u32 magic;
	u32 ver;
	u32 size;

	u32 pause_map;
	u32 tech_map;
	u32 fec_map;
	u32 hpe_map;
};

#define SL_LINK_CONFIG_PAUSE_ASYM        BIT(1)
#define SL_LINK_CONFIG_PAUSE_SYM         BIT(0)

#define SL_LINK_CONFIG_PAUSE_MASK (SL_LINK_CONFIG_PAUSE_ASYM | \
				   SL_LINK_CONFIG_PAUSE_SYM)

#define SL_LINK_CONFIG_HPE_LINKTRAIN     BIT(18)
#define SL_LINK_CONFIG_HPE_PRECODING     BIT(17)
#define SL_LINK_CONFIG_HPE_PCAL          BIT(16)  /* progressive calibration */
/* Rosetta versions */
#define SL_LINK_CONFIG_HPE_R3            BIT(10)
#define SL_LINK_CONFIG_HPE_R2            BIT(9)
#define SL_LINK_CONFIG_HPE_R1            BIT(8)
/* Cassini versions */
#define SL_LINK_CONFIG_HPE_C3            BIT(6)
#define SL_LINK_CONFIG_HPE_C2            BIT(5)
#define SL_LINK_CONFIG_HPE_C1            BIT(4)
/* bit 3 used for credits */
/* bit 2 used for v1 */
/* bit 1 used for v1 */
#define SL_LINK_CONFIG_HPE_LLR           BIT(0)

#define SL_LINK_CONFIG_HPE_MASK (SL_LINK_CONFIG_HPE_LINKTRAIN | \
				 SL_LINK_CONFIG_HPE_PRECODING | \
				 SL_LINK_CONFIG_HPE_PCAL      | \
				 SL_LINK_CONFIG_HPE_R3        | \
				 SL_LINK_CONFIG_HPE_R2        | \
				 SL_LINK_CONFIG_HPE_R1        | \
				 SL_LINK_CONFIG_HPE_C3        | \
				 SL_LINK_CONFIG_HPE_C2        | \
				 SL_LINK_CONFIG_HPE_C1        | \
				 SL_LINK_CONFIG_HPE_LLR)

#define SL_LINK_CONFIG_MAGIC 0x6c6b6366
#define SL_LINK_CONFIG_VER   2
struct sl_link_config {
	u32 magic;
	u32 ver;
	u32 size;

	u32 link_up_timeout_ms;
	u32 link_up_tries_max;

	s32 fec_up_settle_wait_ms;
	s32 fec_up_check_wait_ms;
	s32 fec_up_ucw_limit;
	s32 fec_up_ccw_limit;

	u32 pause_map;
	u32 hpe_map;

	u32 options;
};

#define SL_LINK_CONFIG_OPT_AUTONEG_ENABLE              BIT(0)
#define SL_LINK_CONFIG_OPT_AUTONEG_CONTINUOUS_ENABLE   BIT(1)
#define SL_LINK_CONFIG_OPT_HEADSHELL_LOOPBACK_ENABLE   BIT(2)
#define SL_LINK_CONFIG_OPT_REMOTE_LOOPBACK_ENABLE      BIT(3)
#define SL_LINK_CONFIG_OPT_EXTENDED_REACH_FORCE        BIT(4)
#define SL_LINK_CONFIG_OPT_ALD_ENABLE                  BIT(5) /* Enable Auto Lane Degrade */
/* BIT 30 Reserved */
/* BIT 31 Reserved */

#define SL_LINK_POLICY_MAGIC 0x6c6b706f
#define SL_LINK_POLICY_VER   2
struct sl_link_policy {
	u32 magic;
	u32 ver;
	u32 size;

	s32 fec_mon_ucw_down_limit;
	s32 fec_mon_ucw_warn_limit;
	s32 fec_mon_ccw_down_limit;
	s32 fec_mon_ccw_warn_limit;
	s32 fec_mon_period_ms;

	u32 options;
};

struct sl_link_degrade_info {
	bool is_tx_degrade;
	bool is_rx_degrade;
	u16  tx_link_speed;  /* in Gbps */
	u16  rx_link_speed;  /* in Gbps */
	u8   tx_lane_map;
	u8   rx_lane_map;
};

#define SL_LINK_POLICY_OPT_KEEP_SERDES_UP        BIT(0) /* Keep serdes running when link is down */
#define SL_LINK_POLICY_OPT_USE_UNSUPPORTED_CABLE BIT(1) /* Try to bring the link up even if cable is not supported */
#define SL_LINK_POLICY_OPT_IGNORE_MEDIA_ERROR    BIT(2) /* Try to bring the link up even if media has errors */
/* BIT 30 Reserved */
/* BIT 31 Reserved */

#define SL_LINK_INFINITE_UP_TRIES ~0

#define SL_LINK_DOWN_CAUSE_NONE              0
#define SL_LINK_DOWN_CAUSE_UCW               BIT(1)  /* link up or fec mon UCW limit crossed */
#define SL_LINK_DOWN_CAUSE_LF                BIT(2)  /* link local fault                     */
#define SL_LINK_DOWN_CAUSE_RF                BIT(3)  /* link remote fault                    */
#define SL_LINK_DOWN_CAUSE_DOWN              BIT(4)  /* link down fault                      */
#define SL_LINK_DOWN_CAUSE_UP_TRIES          BIT(5)  /* link up tries exhaused               */
#define SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH   BIT(6)  /* lp_caps autoneg no match             */
#define SL_LINK_DOWN_CAUSE_AUTONEG           BIT(7)  /* autoneg failure                      */
#define SL_LINK_DOWN_CAUSE_CONFIG            BIT(10) /* link up bad config                   */
#define SL_LINK_DOWN_CAUSE_INTR_ENABLE       BIT(11) /* link up interrupt enable failure     */
#define SL_LINK_DOWN_CAUSE_TIMEOUT           BIT(12) /* link up timeout                      */
#define SL_LINK_DOWN_CAUSE_CANCELED          BIT(13) /* link up cancelled                    */
#define SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE BIT(14) /* unsuppported cable                   */
#define SL_LINK_DOWN_CAUSE_COMMAND           BIT(15) /* client command                       */
#define SL_LINK_DOWN_CAUSE_DOWNSHIFT         BIT(16) /* link up cable downshift failed       */
#define SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX    BIT(17) /* LLR replay at max fault              */
#define SL_LINK_DOWN_CAUSE_UPSHIFT           BIT(18) /* link up cable upshift failed         */
#define SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG    BIT(19) /* link up config after an failed       */
#define SL_LINK_DOWN_CAUSE_PCS_FAULT         BIT(20) /* link up PCS is not ok                */
#define SL_LINK_DOWN_CAUSE_SERDES_PLL        BIT(21) /* link up serdes problems              */
#define SL_LINK_DOWN_CAUSE_SERDES_CONFIG     BIT(22) /* link up serdes config problems       */
#define SL_LINK_DOWN_CAUSE_SERDES_SIGNAL     BIT(23) /* link up serdes signal problems       */
#define SL_LINK_DOWN_CAUSE_SERDES_QUALITY    BIT(24) /* link up serdes quality problems      */
#define SL_LINK_DOWN_CAUSE_NO_MEDIA          BIT(25) /* no media present                     */
#define SL_LINK_DOWN_CAUSE_CCW               BIT(26) /* link up or fec mon CCW limit crossed */
#define SL_LINK_DOWN_CAUSE_HIGH_TEMP         BIT(27) /* active cable too hot                 */
#define SL_LINK_DOWN_CAUSE_INTR_REGISTER     BIT(28) /* link up interrupt register failure   */
#define SL_LINK_DOWN_CAUSE_MEDIA_ERROR       BIT(29) /* media has errors                     */

#define SL_LINK_DOWN_RETRYABLE               BIT(61) /* client retry possible                */
#define SL_LINK_DOWN_ORIGIN_ASYNC            BIT(62) /* link down cause was asynchronous     */
#define SL_LINK_DOWN_ORIGIN_LINK_UP          BIT(63) /* link down before reaching up         */

#define SL_LINK_DOWN_CAUSE_STR_SIZE     128
#define SL_LINK_DOWN_CAUSE_STR_SIZE_MIN 4

struct sl_link *sl_link_new(struct sl_lgrp *lgrp, u8 link_num, struct kobject *sysfs_parent);
int             sl_link_del(struct sl_link *link);

int sl_link_state_get(struct sl_link *link, u32 *state);

int sl_link_config_set(struct sl_link *link, struct sl_link_config *link_config);
int sl_link_policy_set(struct sl_link *link, struct sl_link_policy *link_policy);

int sl_link_an_lp_caps_get(struct sl_link *link, struct sl_link_caps *caps, u32 timeout_ms, u32 flags);
int sl_link_an_lp_caps_stop(struct sl_link *link);

int sl_link_up(struct sl_link *link);
int sl_link_down(struct sl_link *link);
int sl_link_reset(struct sl_link *link);

int sl_link_clocks_get(struct sl_link *link, u32 *up_count, s64 *up_time, s64 *total_time);

const char *sl_link_state_str(u32 state);
const char *sl_link_degrade_state_str(int degrade_state);
const char *sl_link_an_lp_caps_state_str(u32 lp_caps_state);
const char *sl_link_config_opt_str(u32 option);
const char *sl_link_policy_opt_str(u32 option);
int         sl_link_down_cause_map_str(u64 cause_map, char *cause_str, unsigned int cause_str_size);
int         sl_link_down_cause_map_with_info_str(u64 cause_map, char *cause_str, unsigned int cause_str_size);
int         sl_link_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size);
const char *sl_link_config_pause_str(u32 config);
const char *sl_link_config_hpe_str(u32 config);

#endif /* _LINUX_SL_LINK_H_ */
