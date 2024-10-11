/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _UAPI_SL_MAC_H_
#define _UAPI_SL_MAC_H_

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

enum sl_mac_state {
	SL_MAC_STATE_OFF,
	SL_MAC_STATE_ON,
};

#endif /* _UAPI_SL_MAC_H_ */
