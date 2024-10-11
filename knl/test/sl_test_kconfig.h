/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_KCONFIG_H_
#define _SL_TEST_KCONFIG_H_

#ifdef BUILDSYS_EXTERNAL_BUILD

/*
 * Rosetta framework
 */
#ifdef BUILDSYS_FRAMEWORK_ROSETTA
#define CONFIG_SL_TEST_FRAMEWORK_ROSETTA y

/*
 * Cassini framework
 */
#elif defined(BUILDSYS_FRAMEWORK_CASSINI)
#define CONFIG_SL_TEST_FRAMEWORK_CASSINI y

/*
 * No framework defined
 */
#else
#error Framework undefined
#endif

/*
 * Real hardware
 */
#define CONFIG_SL_TEST_BUILD_NAME "sshot"

#endif /* BUILDSYS_EXTERNAL_BUILD */

#endif /* _SL_TEST_KCONFIG_H_ */
