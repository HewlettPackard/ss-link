/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _UAPI_SL_LLR_H_
#define _UAPI_SL_LLR_H_

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

#define SL_LLR_LINK_DN_BEHAVIOR_DISCARD     BIT(0)
#define SL_LLR_LINK_DN_BEHAVIOR_BLOCK       BIT(1)
#define SL_LLR_LINK_DN_BEHAVIOR_BEST_EFFORT BIT(2)

#define SL_LLR_CONFIG_MAGIC 0x636c6c72
#define SL_LLR_CONFIG_VER   1
struct sl_llr_config {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 mode;
	__u32 setup_timeout_ms;
	__u32 start_timeout_ms;
	__u32 link_dn_behavior;

	__u32 options;
};

#define SL_LLR_POLICY_OPT_INFINITE_START_TRIES BIT(0)

#define SL_LLR_POLICY_MAGIC 0x636c6c72
#define SL_LLR_POLICY_VER   1
struct sl_llr_policy {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 options;
};

enum sl_llr_state {
	SL_LLR_STATE_OFF,
	SL_LLR_STATE_BUSY,
	SL_LLR_STATE_RUNNING,
};

#endif /* _UAPI_SL_LLR_H_ */
