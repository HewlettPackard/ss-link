/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_PMI_H_
#define _SL_CORE_HW_PMI_H_

#define SL_CORE_HW_PMI_RD(_lgrp, _dev_id, _lane, _pll, _addr, _shl, _shr, _data)                     \
	do {                                                                                         \
		rtn = pmi_rd((_lgrp), (_dev_id), (_lane), (_pll), (_addr), (_shl), (_shr), (_data)); \
		if (rtn != 0)                                                                        \
			goto out;                                                                    \
	} while (0)

#define SL_CORE_HW_PMI_WR(_lgrp, _dev_id, _lane, _pll, _addr, _data, _shl, _mask)                     \
	do {                                                                                          \
		rtn = pmi_wr((_lgrp), (_dev_id), (_lane), (_pll), (_addr), (_data), (_shl), (_mask)); \
		if (rtn != 0)                                                                         \
			goto out;                                                                     \
	} while (0)

struct sl_core_lgrp;

int pmi_rd(struct sl_core_lgrp *core_lgrp, u8 dev_id, u8 lane,
	   u8 pll, u16 addr, u8 shl, u8 shr, u16 *data);

int pmi_wr(struct sl_core_lgrp *core_lgrp, u8 dev_id, u8 lane,
	   u8 pll, u16 addr, u16 data, u8 shl, u16 mask);

#endif /* _SL_CORE_HW_PMI_H_ */
