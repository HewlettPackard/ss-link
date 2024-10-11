/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SETTINGS_H_
#define _SL_CORE_HW_SETTINGS_H_

#define SL_CORE_HW_RS_MODE_OFF                           0x0
#define SL_CORE_HW_RS_MODE_CORRECT                       0x5

#define SL_CORE_HW_PCS_MODE_BS_200G                      0x0
#define SL_CORE_HW_PCS_MODE_BJ_100G                      0x1
#define SL_CORE_HW_PCS_MODE_CD_100G                      0x2
#define SL_CORE_HW_PCS_MODE_CD_50G                       0x3
#define SL_CORE_HW_PCS_MODE_CK_400G                      0x4
#define SL_CORE_HW_PCS_MODE_CK_200G                      0x5
#define SL_CORE_HW_PCS_MODE_CK_100G                      0x6

#define SL_CORE_HW_IFG_ADJ_200G                          0x0
#define SL_CORE_HW_IFG_ADJ_100G                          0x1
#define SL_CORE_HW_IFG_ADJ_50G                           0x2
#define SL_CORE_HW_IFG_ADJ_NONE                          0x3
#define SL_CORE_HW_IFG_ADJ_400G                          0x4

#define SL_CORE_HW_ACTIVE_LANES_BS_200G                  0xF
#define SL_CORE_HW_ACTIVE_LANES_BJ_100G                  0xF
#define SL_CORE_HW_ACTIVE_LANES_CD_100G(_link_num)       (0x3 << ((_link_num) * 2))
#define SL_CORE_HW_ACTIVE_LANES_CD_50G(_link_num)        (0x1 << (_link_num))
#define SL_CORE_HW_ACTIVE_LANES_CK_400G                  0xF
#define SL_CORE_HW_ACTIVE_LANES_CK_200G(_link_num)       (0x3 << ((_link_num) * 2))
#define SL_CORE_HW_ACTIVE_LANES_CK_100G(_link_num)       (0x1 << (_link_num))

#endif /* _SL_CORE_HW_SETTINGS_H_ */
