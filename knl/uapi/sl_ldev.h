/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _UAPI_SL_LDEV_H_
#define _UAPI_SL_LDEV_H_

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

#define SL_LDEV_POLICY_MAGIC 0x736c6470
#define SL_LDEV_POLICY_VER   1
struct sl_ldev_policy {
	__u32 magic;
	__u32 ver;
	__u32 size;

	__u32 options;
};

#endif /* _UAPI_SL_LDEV_H_ */
