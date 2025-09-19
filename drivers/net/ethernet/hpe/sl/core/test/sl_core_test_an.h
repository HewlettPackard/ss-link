/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_TEST_AN_H_
#define _SL_CORE_TEST_AN_H_

struct sl_link;
struct sl_link_caps;

int sl_core_test_an_caps_set(struct sl_link *link, struct sl_link_caps *caps);

#endif /* _SL_CORE_TEST_AN_H_ */
