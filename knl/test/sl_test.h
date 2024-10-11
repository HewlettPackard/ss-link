/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_H_
#define _SL_TEST_H_

#include "sl_test_kconfig.h"

#define SL_TEST_VERSION_MAJOR 1
#define SL_TEST_VERSION_MINOR 0
#define SL_TEST_VERSION_INC   2

void  sl_test_version_get(int *major, int *minor, int *inc);
char *sl_test_git_hash_str_get(void);

#endif /* _SL_TEST_H_ */
