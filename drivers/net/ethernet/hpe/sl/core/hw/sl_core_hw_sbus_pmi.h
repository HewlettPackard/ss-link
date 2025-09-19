/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SBUS_PMI_H_
#define _SL_CORE_HW_SBUS_PMI_H_

#define SL_CORE_HW_SBUS_PMI_RD(_lgrp, _dev_addr, _dev_id, _lane, _sel, _addr, _mask, _data)                     \
	do {                                                                                                    \
		rtn = sbus_pmi_rd((_lgrp), (_dev_addr), (_dev_id), (_lane), (_sel), (_addr), (_mask), (_data)); \
		if (rtn != 0)                                                                                   \
			goto out;                                                                               \
	} while (0)

#define SL_CORE_HW_SBUS_PMI_WR(_lgrp, _dev_addr, _dev_id, _lane, _sel, _addr, _data, _mask)                     \
	do {                                                                                                    \
		rtn = sbus_pmi_wr((_lgrp), (_dev_addr), (_dev_id), (_lane), (_sel), (_addr), (_data), (_mask)); \
		if (rtn != 0)                                                                                   \
			goto out;                                                                               \
	} while (0)

struct sl_core_lgrp;

int sbus_pmi_rd(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 dev_id, u8 lane,
		u8 sel, u16 addr, u16 mask, u16 *data);
int sbus_pmi_wr(struct sl_core_lgrp *core_lgrp, u8 dev_addr, u8 dev_id, u8 lane,
		u8 sel, u16 addr, u16 data, u16 mask);

#endif /* _SL_CORE_HW_SBUS_PMI_H_ */
