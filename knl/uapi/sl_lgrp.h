/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _UAPI_SL_LGRP_H_
#define _UAPI_SL_LGRP_H_

#ifdef __KERNEL__
#include <linux/bitops.h>
#else
#ifndef BIT
#define BIT(_num)      (1UL << (_num))
#endif
#ifndef BIT_ULL
#define BIT_ULL(_num)  (1ULL << (_num))
#endif
#endif

#define SL_LGRP_NOTIF_INVALID           0        /* Not a notification */
#define SL_LGRP_NOTIF_ALL               ~0       /* All notifications  */
#define SL_LGRP_NOTIF_LINK_UP           BIT(1)
#define SL_LGRP_NOTIF_LINK_UP_FAIL      BIT(2)
#define SL_LGRP_NOTIF_LINK_ASYNC_DOWN   BIT(3)
#define SL_LGRP_NOTIF_LINK_ERROR        BIT(4)
#define SL_LGRP_NOTIF_LINK_UCW_WARN     BIT(5)
#define SL_LGRP_NOTIF_LINK_CCW_WARN     BIT(6)
#define SL_LGRP_NOTIF_LLR_DATA          BIT(7)
#define SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT BIT(8)
#define SL_LGRP_NOTIF_LLR_START_TIMEOUT BIT(9)
#define SL_LGRP_NOTIF_LLR_RUNNING       BIT(10)
#define SL_LGRP_NOTIF_LLR_ERROR         BIT(11)
#define SL_LGRP_NOTIF_MEDIA_PRESENT     BIT(12)
#define SL_LGRP_NOTIF_MEDIA_NOT_PRESENT BIT(13)
#define SL_LGRP_NOTIF_MEDIA_ERROR       BIT(14)
#define SL_LGRP_NOTIF_AN_DATA           BIT(15)
#define SL_LGRP_NOTIF_AN_TIMEOUT        BIT(16)
#define SL_LGRP_NOTIF_AN_ERROR          BIT(17)
#define SL_LGRP_NOTIF_LLR_CANCELED      BIT(18)

#define SL_LGRP_NOTIF_NO_LINK 0xFF

#define SL_LGRP_NOTIF_LINK     (SL_LGRP_NOTIF_LINK_UP         | \
				SL_LGRP_NOTIF_LINK_UP_FAIL    | \
				SL_LGRP_NOTIF_LINK_ASYNC_DOWN | \
				SL_LGRP_NOTIF_LINK_ERROR      | \
				SL_LGRP_NOTIF_LINK_UCW_WARN   | \
				SL_LGRP_NOTIF_LINK_CCW_WARN)

#define SL_LGRP_NOTIF_LLR      (SL_LGRP_NOTIF_LLR_DATA | \
				SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT | \
				SL_LGRP_NOTIF_LLR_START_TIMEOUT | \
				SL_LGRP_NOTIF_LLR_CANCELED      | \
				SL_LGRP_NOTIF_LLR_RUNNING       | \
				SL_LGRP_NOTIF_LLR_ERROR)

#define SL_LGRP_NOTIF_MEDIA    (SL_LGRP_NOTIF_MEDIA_PRESENT     | \
				SL_LGRP_NOTIF_MEDIA_NOT_PRESENT | \
				SL_LGRP_NOTIF_MEDIA_ERROR)

#define SL_LGRP_NOTIF_AN       (SL_LGRP_NOTIF_AN_DATA    | \
				SL_LGRP_NOTIF_AN_TIMEOUT | \
				SL_LGRP_NOTIF_AN_ERROR)

#define SL_LGRP_CONFIG_TECH_CK_400G      BIT(18)
#define SL_LGRP_CONFIG_TECH_CK_200G      BIT(17)
#define SL_LGRP_CONFIG_TECH_CK_100G      BIT(16)
#define SL_LGRP_CONFIG_TECH_BS_200G      BIT(15)
#define SL_LGRP_CONFIG_TECH_CD_100G      BIT(14)
#define SL_LGRP_CONFIG_TECH_CD_50G       BIT(13)
#define SL_LGRP_CONFIG_TECH_BJ_100G      BIT(8)

#define SL_LGRP_CONFIG_TECH_MASK (SL_LGRP_CONFIG_TECH_CK_400G | \
				  SL_LGRP_CONFIG_TECH_CK_200G | \
				  SL_LGRP_CONFIG_TECH_CK_100G | \
				  SL_LGRP_CONFIG_TECH_BS_200G | \
				  SL_LGRP_CONFIG_TECH_CD_100G | \
				  SL_LGRP_CONFIG_TECH_CD_50G  | \
				  SL_LGRP_CONFIG_TECH_BJ_100G)

#define SL_LGRP_CONFIG_FEC_RS_LL         BIT(1) // FIXME: need to fix how we handle this
#define SL_LGRP_CONFIG_FEC_RS            BIT(0)

#define SL_LGRP_CONFIG_FEC_MASK (SL_LGRP_CONFIG_FEC_RS_LL | \
				 SL_LGRP_CONFIG_FEC_RS)

#define SL_LGRP_FEC_MODE_OFF     BIT(0)
#define SL_LGRP_FEC_MODE_MONITOR BIT(1)
#define SL_LGRP_FEC_MODE_CORRECT BIT(2)

#define SL_LGRP_LLR_MODE_OFF     BIT(0)
#define SL_LGRP_LLR_MODE_MONITOR BIT(1)
#define SL_LGRP_LLR_MODE_ON      BIT(2)

#define SL_LGRP_CONFIG_MAGIC 0x6c676366
#define SL_LGRP_CONFIG_VER   1
struct sl_lgrp_config {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 mfs;
	__u32 furcation;
	__u32 fec_mode;

	__u32 tech_map;
	__u32 fec_map;

	__u32 options;
};

#define SL_LGRP_OPT_FABRIC BIT(0) /* fabric link */
#define SL_LGRP_OPT_R1     BIT(1) /* connected to R1 */

#define SL_LGRP_POLICY_MAGIC 0x6c67706f
#define SL_LGRP_POLICY_VER   1
struct sl_lgrp_policy {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 options;
};

#endif /* _UAPI_SL_LGRP_H_ */
