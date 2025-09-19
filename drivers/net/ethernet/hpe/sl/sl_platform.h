/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_PLATFORM_H_
#define _SL_PLATFORM_H_

#define SL_PLATFORM_IS_HARDWARE(_ldev) \
	((_ldev)->platform == PLATFORM_ASIC)

#define SL_PLATFORM_IS_NETSIM(_ldev)   \
	((_ldev)->platform == PLATFORM_NETSIM)

#define SL_PLATFORM_IS_EMULATOR(_ldev) \
	((_ldev)->platform == PLATFORM_Z1)

#endif /* _SL_PLATFORM_H_ */
