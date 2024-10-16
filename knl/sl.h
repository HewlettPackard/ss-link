/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_H_
#define _SL_H_

#define SL_VERSION_MAJOR 1
#define SL_VERSION_MINOR 20
#define SL_VERSION_INC   1

void  sl_version_get(int *major, int *minor, int *inc);
char *sl_git_hash_str_get(void);

#endif /* _SL_H_ */
