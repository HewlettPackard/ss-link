/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_KCONFIG_H_
#define _SL_KCONFIG_H_

#ifdef BUILDSYS_EXTERNAL_BUILD

#define CONFIG_SL                        y

#ifdef SL_TEST
#define CONFIG_SL_FRAMEWORK_TEST         y
#endif

#ifdef BUILDSYS_FRAMEWORK_SW2
#define CONFIG_SL_FRAMEWORK_SW2          y
#define CONFIG_SL_DEFAULT_WQ_MAX_ACTIVE  0 /* 0 is default (256 work items in queue). */

#elif defined(BUILDSYS_FRAMEWORK_NIC2)
#define CONFIG_SL_FRAMEWORK_NIC2         y
#define CONFIG_SL_DEFAULT_WQ_MAX_ACTIVE  0 /* 0 is default (256 work items in queue). */

#else
#error Framework undefined
#endif

#endif /* BUILDSYS_EXTERNAL_BUILD */

#endif /* _SL_KCONFIG_H_ */
