/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

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

#define SL_LINK_DOWN_RETRYABLE               BIT(61) /* client retry possible               */
#define SL_LINK_DOWN_ORIGIN_ASYNC            BIT(62) /* link down cause was asynchronous    */
#define SL_LINK_DOWN_ORIGIN_LINK_UP          BIT(63) /* link down before reaching up        */

#define SL_LINK_DOWN_CAUSE_STR_SIZE 128

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
int         sl_link_down_cause_map_str(u64 cause_map, char *cause_str, unsigned int cause_str_size);
int         sl_link_down_cause_map_with_info_str(u64 cause_map, char *cause_str, unsigned int cause_str_size);
int         sl_link_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size);
const char *sl_link_config_pause_str(u32 config);
const char *sl_link_config_hpe_str(u32 config);

#endif /* _LINUX_SL_LINK_H_ */
