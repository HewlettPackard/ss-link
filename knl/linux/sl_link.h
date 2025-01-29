/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_LINK_H_
#define _LINUX_SL_LINK_H_

#include "uapi/sl_link.h"

struct sl_lgrp;
struct sl_link;
struct sl_link_config;
struct sl_link_policy;
struct sl_link_caps;
struct kobject;

#define SL_LINK_INFINITE_UP_TRIES ~0

#define SL_LINK_DOWN_CAUSE_NONE              0
#define SL_LINK_DOWN_CAUSE_BAD_EYE           BIT(1)   /* eye limits not good      */
#define SL_LINK_DOWN_CAUSE_UCW               BIT(2)   /* UCW limit crossed        */
#define SL_LINK_DOWN_CAUSE_CCW               BIT(3)   /* CCW limit crossed        */
#define SL_LINK_DOWN_CAUSE_ALIGN             BIT(4)   /* link alignment failure   */
#define SL_LINK_DOWN_CAUSE_LF                BIT(5)   /* link local fault         */
#define SL_LINK_DOWN_CAUSE_RF                BIT(6)   /* link remote fault        */
#define SL_LINK_DOWN_CAUSE_SERDES            BIT(7)   /* serdes problems          */
#define SL_LINK_DOWN_CAUSE_DOWN              BIT(8)   /* link down                */
#define SL_LINK_DOWN_CAUSE_UP_TRIES          BIT(9)   /* up tries exhaused        */
#define SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH   BIT(10)  /* no match for autoneg     */
#define SL_LINK_DOWN_CAUSE_AUTONEG_FAIL      BIT(11)  /* autoneg failure          */
#define SL_LINK_DOWN_CAUSE_CONFIG            BIT(12)  /* bad config               */
#define SL_LINK_DOWN_CAUSE_INTR_ENABLE       BIT(13)  /* failure enable interrupt */
#define SL_LINK_DOWN_CAUSE_TIMEOUT           BIT(14)  /* link action timeout      */
#define SL_LINK_DOWN_CAUSE_CANCELED          BIT(15)  /* link action cancelled    */
#define SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE BIT(16)  /* unsuppported cable       */
#define SL_LINK_DOWN_CAUSE_COMMAND           BIT(17)  /* client command           */
#define SL_LINK_DOWN_CAUSE_DOWNSHIFT_FAILED  BIT(18)  /* downshift failed         */
#define SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX    BIT(19)  /* LLR replay at max        */
#define SL_LINK_DOWN_CAUSE_UPSHIFT_FAILED    BIT(20)  /* upshift failed           */

#define SL_LINK_DATA_STATUS_BIT_LOCK        BIT(0)
#define SL_LINK_DATA_STATUS_BIT_ALIGN       BIT(1)
#define SL_LINK_DATA_STATUS_BIT_HISER       BIT(2)
#define SL_LINK_DATA_STATUS_BIT_FAULT       BIT(3)
#define SL_LINK_DATA_STATUS_BIT_LOCAL_FAULT BIT(4)

struct sl_link_data {
	u8 active_lanes;
	u8 good_eyes;
	u8 not_idle;
	u8 status;         /* SL_LINK_DATA_STATUS */
};

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

int sl_link_clocks_get(struct sl_link *link, u32 *up_count, u32 *up_time, u32 *total_time);

const char *sl_link_state_str(u32 state);
const char *sl_link_config_opt_str(u32 option);
const char *sl_link_policy_opt_str(u32 option);
void        sl_link_down_cause_str(u32 cause, char *cause_str, unsigned int cause_str_size);
int         sl_link_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size);
const char *sl_link_config_pause_str(u32 config);
const char *sl_link_config_hpe_str(u32 config);

#endif /* _LINUX_SL_LINK_H_ */
