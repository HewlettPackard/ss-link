/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_ASIC_H_
#define _SL_ASIC_H_

#include <linux/types.h>

#include "sl_kconfig.h"

#define SL_PLATFORM_IS_HARDWARE(_ldev) ((_ldev)->platform == PLATFORM_ASIC)
#define SL_PLATFORM_IS_NETSIM(_ldev)   ((_ldev)->platform == PLATFORM_NETSIM)
#define SL_PLATFORM_IS_EMULATOR(_ldev) ((_ldev)->platform == PLATFORM_Z1)

#define SL_MAX_SERDES_LANES 8

#ifdef BUILDSYS_FRAMEWORK_SW2

#include "ss2_port_pml.h"
#include "r2_csr_memorg.h"
#include "r2_counters_def.h"

/* Currently we don't use r2_cntr_descs_str */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#include "rosetta2_user_defs.h"
#pragma GCC diagnostic pop

#define SL_ASIC_MAX_LDEVS  1
#define SL_ASIC_MAX_LGRPS  64
#define SL_ASIC_MAX_LINKS  4
#define SL_ASIC_MAX_LANES  4
#define SL_ASIC_MAX_SERDES 32

#define SS2_PORT_BASE(port) SS2_PORT_PML_BASE(port)

// FIXME: temporary addition in anticipation of CSR changes
#define PORT_PML_CFG_PORT_GROUP	(SS2_PORT_PML_BASE(port) + SS2_PORT_PML_CFG_PORT_GROUP_OFFSET)

#define SL_MEDIA_MAX_JACK_NUM          32
#define SL_MEDIA_MAX_LGRPS_PER_JACK    4
#define SL_MEDIA_LGRPS_PER_PORT        2
#define SL_MEDIA_LGRPS_DIVIDE_PER_PORT (SL_MEDIA_LGRPS_PER_PORT >> 1)

#define SL_SERDES_NUM_MICROS    4
#define SL_SERDES_ACTIVE_MICROS (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SL_SERDES_NUM_PLLS      1
#define SL_SERDES_NUM_LANES     8

#else /* NIC2 */

#include "sbl/sbl_pml.h"

// FIXME: temporary addition in anticipation of CSR changes
#define PORT_PML_CFG_PORT_GROUP	SS2_PORT_PML_CFG_PORT_GROUP
#define SS2_PORT_PML_CFG_PORT_GROUP_PG_CFG_UPDATE(_a, _b) C_UPDATE(_a, _b, ss2_port_pml_cfg_port_group, pg_cfg)

#define SL_ASIC_MAX_LDEVS  16
#define SL_ASIC_MAX_LGRPS  1
#define SL_ASIC_MAX_LINKS  1
#define SL_ASIC_MAX_LANES  4
#define SL_ASIC_MAX_SERDES 1

#define PLATFORM_ASIC   C_PLATFORM_ASIC
#define PLATFORM_NETSIM C_PLATFORM_NETSIM
#define PLATFORM_Z1     C_PLATFORM_Z1

#define SL_MEDIA_MAX_JACK_NUM          1
#define SL_MEDIA_MAX_LGRPS_PER_JACK    1
#define SL_MEDIA_LGRPS_PER_PORT        1
#define SL_MEDIA_LGRPS_DIVIDE_PER_PORT 0

#define SL_SERDES_NUM_MICROS    2
#define SL_SERDES_ACTIVE_MICROS (BIT(1) | BIT(0))
#define SL_SERDES_NUM_PLLS      1
#define SL_SERDES_NUM_LANES     4

#endif /* BUILDSYS_FRAMEWORK_SW2 */

#endif /* _SL_ASIC_H_ */
