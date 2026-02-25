/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_PLATFORM_H_
#define _SL_PLATFORM_H_
#include <linux/version.h>

#define SL_PLATFORM_IS_HARDWARE(_ldev) \
	((_ldev)->platform == PLATFORM_ASIC)

#define SL_PLATFORM_IS_NETSIM(_ldev)   \
	((_ldev)->platform == PLATFORM_NETSIM)

#define SL_PLATFORM_IS_EMULATOR(_ldev) \
	((_ldev)->platform == PLATFORM_Z1)

#ifndef timer_container_of
#define timer_container_of from_timer
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 15, 0)
#define timer_delete_sync del_timer_sync
#endif

#endif /* _SL_PLATFORM_H_ */
