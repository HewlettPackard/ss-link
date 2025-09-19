/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_ASIC_H_
#define _SL_ASIC_H_

// FIXME: all the CSR things will go in a central place
#include "ss2_port_pml.h"
#include "r2_csr_memorg.h"
#include "r2_counters_def.h"

/* Currently we don't use r2_cntr_descs_str */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#include "rosetta2_user_defs.h"
#pragma GCC diagnostic pop

#define SS2_PORT_BASE(port) SS2_PORT_PML_BASE(port)
// FIXME: temporary addition in anticipation of CSR changes
#define PORT_PML_CFG_PORT_GROUP	(SS2_PORT_PML_BASE(port) + SS2_PORT_PML_CFG_PORT_GROUP_OFFSET)

// FIXME: leave everything below in this file

#define SL_MAX_SERDES_LANES 8

#define SL_ASIC_MAX_LDEVS  1
#define SL_ASIC_MAX_LGRPS  64
#define SL_ASIC_MAX_LINKS  4
#define SL_ASIC_MAX_LANES  4
#define SL_ASIC_MAX_SERDES 32

#define SL_MEDIA_MAX_JACK_NUM          32
#define SL_MEDIA_MAX_LGRPS_PER_JACK    4
#define SL_MEDIA_LGRPS_PER_PORT        2
#define SL_MEDIA_LGRPS_DIVIDE_PER_PORT (SL_MEDIA_LGRPS_PER_PORT >> 1)

#define SL_SERDES_NUM_MICROS    4
#define SL_SERDES_ACTIVE_MICROS (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SL_SERDES_NUM_PLLS      1
#define SL_SERDES_NUM_LANES     8

#endif /* _SL_ASIC_H_ */
