/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_ASIC_H_
#define _SL_ASIC_H_

#include "sl_asic_csrs.h"

// FIXME: temporary addition in anticipation of CSR changes
#define PORT_PML_CFG_PORT_GROUP	SS2_PORT_PML_CFG_PORT_GROUP

// FIXME: the platform macros need to go in platform

#define PLATFORM_ASIC   C_PLATFORM_ASIC
#define PLATFORM_NETSIM C_PLATFORM_NETSIM
#define PLATFORM_Z1     C_PLATFORM_Z1

// FIXME: leave everything below in this file

#define SL_MAX_SERDES_LANES 4

#define SL_ASIC_MAX_LDEVS  16
#define SL_ASIC_MAX_LGRPS  1
#define SL_ASIC_MAX_LINKS  1
#define SL_ASIC_MAX_LANES  4
#define SL_ASIC_MAX_SERDES 1

#define SL_MEDIA_MAX_JACK_NUM          1
#define SL_MEDIA_MAX_LGRPS_PER_JACK    1
#define SL_MEDIA_LGRPS_PER_PORT        1
#define SL_MEDIA_LGRPS_DIVIDE_PER_PORT 0

#define SL_SERDES_NUM_MICROS    2
#define SL_SERDES_ACTIVE_MICROS (BIT(1) | BIT(0))
#define SL_SERDES_NUM_PLLS      1
#define SL_SERDES_NUM_LANES     4

//FIXME: Remove when SS3 changes merged in SSHOTPFCMN-218
#define PLATFORM_NIC (SL_MAX_SERDES_LANES == 4)

#endif /* _SL_ASIC_H_ */
