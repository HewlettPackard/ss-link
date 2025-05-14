/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LINK_AN_H_
#define _SL_CORE_LINK_AN_H_

struct sl_link_caps;

#define SL_CORE_LINK_LP_CAPS_RUNNING     BIT(0)
#define SL_CORE_LINK_LP_CAPS_DATA        BIT(1)
#define SL_CORE_LINK_LP_CAPS_TIMEOUT     BIT(2)
#define SL_CORE_LINK_LP_CAPS_ERROR       BIT(3)
#define SL_CORE_LINK_LP_CAPS_NOT_RUNNING BIT(4)

#define SL_CORE_LINK_AN_MAX_PAGES 10

#define SL_CORE_LINK_AN_MAGIC 0x736c414E

typedef int (*sl_core_link_an_callback_t)(void *tag, struct sl_link_caps *caps, u32 result);

int  sl_core_link_an_lp_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				 sl_core_link_an_callback_t callback, void *tag,
				 struct sl_link_caps *caps, u32 timeout_ms, u32 flags);
int  sl_core_link_an_lp_caps_stop(u8 ldev_num, u8 lgrp_num, u8 link_num);

u32  sl_core_link_an_lp_caps_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num);
void sl_core_link_an_fail_cause_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *fail_cause, time64_t *fail_time);
const char *sl_core_link_an_fail_cause_str(u32 fail_cause);

#endif /* _SL_CORE_LINK_AN_H_ */
