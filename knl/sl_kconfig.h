/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_KCONFIG_H_
#define _SL_KCONFIG_H_

#ifdef BUILDSYS_EXTERNAL_BUILD

#define CONFIG_SL                        y

#ifdef SL_TEST
#define CONFIG_SL_FRAMEWORK_TEST         y
#endif

/*
 * Rosetta
 */
#ifdef BUILDSYS_FRAMEWORK_ROSETTA
#define CONFIG_SL_FRAMEWORK_ROSETTA      y
#define CONFIG_SL_DEFAULT_WQ_MAX_ACTIVE  0 /* 0 is default (256 work items in queue). */

/*
 * Cassini
 */
#elif defined(BUILDSYS_FRAMEWORK_CASSINI)
#define CONFIG_SL_FRAMEWORK_CASSINI      y
#define CONFIG_SL_DEFAULT_WQ_MAX_ACTIVE  0 /* 0 is default (256 work items in queue). */

/*
 * No framework defined
 */
#else
#error Framework undefined
#endif

#endif /* BUILDSYS_EXTERNAL_BUILD */

#endif /* _SL_KCONFIG_H_ */
