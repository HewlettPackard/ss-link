/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_KCONFIG_H_
#define _SL_TEST_KCONFIG_H_

#ifdef BUILDSYS_EXTERNAL_BUILD

#ifdef BUILDSYS_FRAMEWORK_SW2
#define CONFIG_SL_TEST_FRAMEWORK_SW2   y

#elif defined(BUILDSYS_FRAMEWORK_NIC2)
#define CONFIG_SL_TEST_FRAMEWORK_NIC2  y

#else
#error Framework undefined
#endif

/*
 * Real hardware
 */
#define CONFIG_SL_TEST_BUILD_NAME "sshot"

#endif /* BUILDSYS_EXTERNAL_BUILD */

#endif /* _SL_TEST_KCONFIG_H_ */
