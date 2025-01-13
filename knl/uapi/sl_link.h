/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _UAPI_SL_LINK_H_
#define _UAPI_SL_LINK_H_

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

#define SL_LINK_INFO_STRLEN 128

enum sl_link_state {
	SL_LINK_STATE_INVALID,
	SL_LINK_STATE_ERROR,    //FIXME: Unused state, remove.
	SL_LINK_STATE_DOWN,
	SL_LINK_STATE_AN,       //FIXME: Unused state, remove.
	SL_LINK_STATE_STARTING,
	SL_LINK_STATE_UP,
	SL_LINK_STATE_STOPPING,
};

#define SL_LINK_CAPS_MAGIC 0x6D676774
#define SL_LINK_CAPS_VER   1
struct sl_link_caps {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 pause_map;
	__u32 tech_map;
	__u32 fec_map;
	__u32 hpe_map;
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
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 link_up_timeout_ms;
	__u32 link_up_tries_max;

	__u32 fec_up_settle_wait_ms;
	__u32 fec_up_check_wait_ms;
	__s32 fec_up_ucw_limit;
	__s32 fec_up_ccw_limit;

	__u32 pause_map;
	__u32 hpe_map;

	__u32 options;
};

#define SL_LINK_CONFIG_OPT_AUTONEG_ENABLE              BIT(0)
#define SL_LINK_CONFIG_OPT_AUTONEG_CONTINUOUS_ENABLE   BIT(1)
#define SL_LINK_CONFIG_OPT_SERDES_LOOPBACK_ENABLE      BIT(2)
#define SL_LINK_CONFIG_OPT_HEADSHELL_LOOPBACK_ENABLE   BIT(3)
#define SL_LINK_CONFIG_OPT_REMOTE_LOOPBACK_ENABLE      BIT(4)
#define SL_LINK_CONFIG_OPT_EXTENDED_REACH_FORCE        BIT(5)
/* BIT 30 Reserved */
/* BIT 31 Reserved */

#define SL_LINK_POLICY_MAGIC 0x6c6b706f
#define SL_LINK_POLICY_VER   2
struct sl_link_policy {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__s32 fec_mon_ucw_down_limit;
	__s32 fec_mon_ucw_warn_limit;
	__s32 fec_mon_ccw_down_limit;
	__s32 fec_mon_ccw_warn_limit;
	__u32 fec_mon_period_ms;

	__u32 options;
};

#define SL_LINK_POLICY_OPT_KEEP_SERDES_UP        BIT(0) /* Keep serdes running when link is down */
#define SL_LINK_POLICY_OPT_USE_UNSUPPORTED_CABLE BIT(1) /* Try to bring the link up even if cable is not supported*/
/* BIT 30 Reserved */
/* BIT 31 Reserved */

#endif /* _UAPI_SL_LINK_H_ */
