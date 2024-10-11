/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_ASIC_H_
#define _SL_ASIC_H_

#include <linux/types.h>

#include "sl_kconfig.h"

#define SL_PLATFORM_IS_HARDWARE(_ldev) ((_ldev)->platform == PLATFORM_ASIC)
#define SL_PLATFORM_IS_NETSIM(_ldev)   ((_ldev)->platform == PLATFORM_NETSIM)
#define SL_PLATFORM_IS_EMULATOR(_ldev) ((_ldev)->platform == PLATFORM_Z1)

#define SL_BOARD_WASHINGTON 4
#define SL_BOARD_KENNEBEC   5

#ifdef BUILDSYS_FRAMEWORK_ROSETTA

#include "ss2_port_pml.h"
#include "r2_csr_memorg.h"
#include "r2_counters_def.h"

/* Currently we don't use r2_cntr_descs_str */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#include "rosetta2_user_defs.h"
#pragma GCC diagnostic pop

#define SL_ASIC_MAX_LDEVS  1
#define SL_ASIC_MAX_LGRPS 64
#define SL_ASIC_MAX_LINKS  4
#define SL_ASIC_MAX_LANES  4

#define SS2_PORT_BASE(port) SS2_PORT_PML_BASE(port)

#define SL_MEDIA_MAX_JACK_NUM           32
#define SL_MEDIA_MAX_LGRPS_PER_JACK     4
#define SL_MEDIA_LGRPS_PER_PORT         2
#define SL_MEDIA_LGRPS_DIVIDE_PER_PORT  (SL_MEDIA_LGRPS_PER_PORT >> 1)

#else /* Cassini */

#include "sbl/sbl_pml.h"

#define SL_ASIC_MAX_LDEVS 16
#define SL_ASIC_MAX_LGRPS 1
#define SL_ASIC_MAX_LINKS 1
#define SL_ASIC_MAX_LANES 4

#define PLATFORM_ASIC   C_PLATFORM_ASIC
#define PLATFORM_NETSIM C_PLATFORM_NETSIM
#define PLATFORM_Z1     C_PLATFORM_Z1

#define SL_MEDIA_MAX_JACK_NUM               1
#define SL_MEDIA_MAX_LGRPS_PER_JACK         1
#define SL_MEDIA_LGRPS_PER_PORT             1
#define SL_MEDIA_LGRPS_DIVIDE_PER_PORT      0

#endif /* BUILDSYS_FRAMEWORK_ROSETTA */

#endif /* _SL_ASIC_H_ */
