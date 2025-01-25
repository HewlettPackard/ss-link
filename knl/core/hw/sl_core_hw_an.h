/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_AN_H_
#define _SL_CORE_HW_AN_H_

struct sl_core_link;
struct work_struct;

/* base page */
#define SL_CORE_HW_AN_BP_BIT_FEC_25G_BASE_R        45
#define SL_CORE_HW_AN_BP_BIT_FEC_25G_RS            44
#define SL_CORE_HW_AN_BP_BIT_FEC_100G_RS           43
#define SL_CORE_HW_AN_BP_BIT_400G_BASE_xR_4        39
#define SL_CORE_HW_AN_BP_BIT_200G_BASE_xR_2        38
#define SL_CORE_HW_AN_BP_BIT_100G_BASE_xR_1        37
#define SL_CORE_HW_AN_BP_BIT_200G_BASE_xR_4        36
#define SL_CORE_HW_AN_BP_BIT_100G_BASE_xR_2        35
#define SL_CORE_HW_AN_BP_BIT_50G_BASE_xR           34
#define SL_CORE_HW_AN_BP_BIT_25G_BASE_KR           31
#define SL_CORE_HW_AN_BP_BIT_25G_BASE_KR_S         30
#define SL_CORE_HW_AN_BP_BIT_100G_BASE_CR_4        29
#define SL_CORE_HW_AN_BP_BIT_100G_BASE_KR_4        28
#define SL_CORE_HW_AN_BP_BIT_TRANS_NONCE           16
#define SL_CORE_HW_AN_BP_BIT_NP                    15
#define SL_CORE_HW_AN_BP_BIT_ACK                   14
#define SL_CORE_HW_AN_BP_BIT_RF                    13
#define SL_CORE_HW_AN_BP_BIT_PAUSE_ASYMETRIC       11
#define SL_CORE_HW_AN_BP_BIT_PAUSE_SYMETRIC        10
#define SL_CORE_HW_AN_BP_BIT_SELECTOR               0
#define SL_CORE_HW_AN_BP_802_3                     (BIT_ULL(0) << SL_CORE_HW_AN_BP_BIT_SELECTOR)

/* next page */
#define SL_CORE_HW_AN_NP_BIT_OUI_ASIC_VER          32
#define SL_CORE_HW_AN_NP_BIT_OUI_OPTS              24
#define SL_CORE_HW_AN_NP_BIT_OUI_OPT_LLR           (0 + SL_CORE_HW_AN_NP_BIT_OUI_OPTS)
#define SL_CORE_HW_AN_NP_BIT_OUI_OPT_ETHER_LLR     (1 + SL_CORE_HW_AN_NP_BIT_OUI_OPTS)
#define SL_CORE_HW_AN_NP_BIT_OUI_OPT_HPC_WITH_LLR  (2 + SL_CORE_HW_AN_NP_BIT_OUI_OPTS)
#define SL_CORE_HW_AN_NP_BIT_OUI                   16
#define SL_CORE_HW_AN_NP_BIT_NP                    15
#define SL_CORE_HW_AN_NP_BIT_ACK                   14
#define SL_CORE_HW_AN_NP_BIT_MP                    13
#define SL_CORE_HW_AN_NP_BIT_ACK2                  12
#define SL_CORE_HW_AN_NP_BIT_T                     11
#define SL_CORE_HW_AN_NP_MSG_NULL                  (BIT_ULL(0) | (BIT_ULL(0) << SL_CORE_HW_AN_NP_BIT_MP))
#define SL_CORE_HW_AN_NP_MSG_OUI                   0x5ULL
#define SL_CORE_HW_AN_NP_MSG_OUI_EXTD              0xBULL
#define SL_CORE_HW_AN_NP_MSG_MASK                  0x7FFULL
#define SL_CORE_HW_AN_NP_OUI_HPE                   (0xEC9B8BULL << SL_CORE_HW_AN_NP_BIT_OUI)
#define SL_CORE_HW_AN_NP_OUI_HPE_MASK              (0xFFFFFFULL << SL_CORE_HW_AN_NP_BIT_OUI)
#define SL_CORE_HW_AN_NP_OUI_VER_0_1               (0x01ULL << SL_CORE_HW_AN_NP_BIT_OUI)
#define SL_CORE_HW_AN_NP_OUI_VER_0_2               (0x02ULL << SL_CORE_HW_AN_NP_BIT_OUI)
#define SL_CORE_HW_AN_NP_OUI_VER_MASK              (0xFFULL << SL_CORE_HW_AN_NP_BIT_OUI)

/* link caps */
#define SL_CORE_HW_AN_BP_PAUSE_SHIFT 10
#define SL_CORE_HW_AN_BP_TECH_SHIFT  21
#define SL_CORE_HW_AN_BP_FEC_SHIFT   43
#define SL_CORE_HW_AN_NP_HPE_SHIFT   24

enum sl_core_hw_an_state {
	SL_CORE_HW_AN_STATE_START     = 0,
	SL_CORE_HW_AN_STATE_BASE,
	SL_CORE_HW_AN_STATE_NEXT,
	SL_CORE_HW_AN_STATE_RETRY,
	SL_CORE_HW_AN_STATE_COMPLETE,
	SL_CORE_HW_AN_STATE_LP_DONE,
	SL_CORE_HW_AN_STATE_ERROR,
};

void sl_core_hw_an_intr_hdlr(u64 *err_flgs, int num_err_flgs, void *data);

void sl_core_hw_an_tx_pages_encode(struct sl_core_link *core_link,
				   struct sl_link_caps *link_caps);
void sl_core_hw_an_stop(struct sl_core_link *core_link);
void sl_core_hw_an_init(struct sl_core_link *core_link);
void sl_core_hw_an_config(struct sl_core_link *core_link);
int  sl_core_hw_an_base_page_send(struct sl_core_link *core_link);
int  sl_core_hw_an_rx_pages_decode(struct sl_core_link *core_link,
				   struct sl_link_caps *link_caps);

#endif /* _SL_CORE_HW_AN_H_ */
