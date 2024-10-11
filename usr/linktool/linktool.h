/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_H_
#define _LINKTOOL_H_

#include "stdio.h"

// FIXME: make debug a level
#define DEBUG(_d, _s, ...) if (_d != 0) printf("DEBUG: " _s "\n", ##__VA_ARGS__)
#define ERROR(_s, ...)                  printf("ERROR: " _s "\n", ##__VA_ARGS__)
#define INFO(_s, ...)                   printf(_s "\n", ##__VA_ARGS__)

#endif /* _LINKTOOL_H_ */
