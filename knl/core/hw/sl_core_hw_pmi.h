/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_PMI_H_
#define _SL_CORE_HW_PMI_H_

#define PMI_DEV_ID_MASK  0x1F
#define PMI_DEV_ID_SHIFT 27
#define PMI_PLL_MASK     0x07
#define PMI_PLL_SHIFT    24
#define PMI_LANE_MASK    0xFF
#define PMI_LANE_SHIFT   16
#define PMI_ADDR_MASK    0xFFFF

#define PMI_ADDR32(_dev_id, _pll, _lane, _addr)              \
	(((_dev_id & PMI_DEV_ID_MASK) << PMI_DEV_ID_SHIFT) | \
	 ((_pll    & PMI_PLL_MASK)    << PMI_PLL_SHIFT)    | \
	 ((_lane   & PMI_LANE_MASK)   << PMI_LANE_SHIFT)   | \
	  (_addr   & PMI_ADDR_MASK))

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
