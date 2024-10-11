/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LINK_FEC_H_
#define _SL_CTL_LINK_FEC_H_

struct sl_ber;
struct sl_fec_info;
struct sl_fec_tails;

int sl_ctl_link_fec_info_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			     struct sl_fec_info *fec_info);
int sl_ctl_link_fec_tail_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			     struct sl_fec_tails *fec_tail);
int sl_ctl_link_fec_ber_calc(struct sl_fec_info *fec_info,
			     struct sl_ber *ucw_ber, struct sl_ber *ccw_ber);

#endif /* _SL_CTL_LINK_FEC_H_ */
