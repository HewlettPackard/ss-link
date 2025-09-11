/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_ASIC_CSRS_H_
#define _SL_ASIC_CSRS_H_

#include "cassini_user_defs.h"
#include "cassini_csr_defaults.h"
#include "cassini_cntr_defs.h"

//==============================================================================

#define SL_CSR_MASK(_type, _bitfield) \
	({                            \
	uint64_t mask = 0;            \
	union _type t = {0};          \
	t._bitfield = -1;             \
	mask = t.qw;                  \
	mask;                         \
	})
#define SL_CSR_WORD_MASK(_type, _bitfield, _word) \
	({                                        \
	uint64_t mask = 0;                        \
	union _type t = {0};                      \
	t._bitfield = -1;                         \
	mask = t.qw[_word];                       \
	mask;                                     \
	})
#define SL_CSR_SHIFT(_type, _bitfield)     \
	({                                 \
	int lsb = 0;                       \
	union _type t = {0};               \
	t._bitfield = 1;                   \
	while (((t.qw >> lsb) & 1) == 0) { \
		lsb++;                     \
	}                                  \
	lsb;                               \
	})
#define SL_CSR_WORD_SHIFT(_type, _bitfield, _word)  \
	({                                          \
	int lsb = 0;                                \
	union _type t = {0};                        \
	t._bitfield = 1;                            \
	while (((t.qw[_word] >> lsb) & 1) == 0) {   \
		lsb++;                              \
	}                                           \
	lsb;                                        \
	})

#define SL_CSR_SET(value, _type, _bitfield) \
	((((unsigned long long)(value)) << SL_CSR_SHIFT(_type, _bitfield)) & SL_CSR_MASK(_type, _bitfield))
#define SL_CSR_UPDATE(val, value, _type, _bitfield) \
	(((val) & (~SL_CSR_MASK(_type, _bitfield))) | SL_CSR_SET(value, _type, _bitfield))
#define SL_CSR_GET(value, _type, _bitfield) \
	(((value) & SL_CSR_MASK(_type, _bitfield)) >> SL_CSR_SHIFT(_type, _bitfield))
#define SL_CSR_WORD_GET(value, _type, _bitfield, _word) \
	(((value) & SL_CSR_WORD_MASK(_type, _bitfield, _word)) >> SL_CSR_WORD_SHIFT(_type, _bitfield, _word))

//==============================================================================

#define SS2_PORT_PML_CFG_PORT_GROUP_PG_CFG_UPDATE(a, b)                              SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_port_group, pg_cfg)

#define SS2_PORT_PML_CFG_SUBPORT_RESET_WARM_RST_FROM_CSR_SET(a)                      SL_CSR_SET(a, ss2_port_pml_cfg_subport_reset, warm_rst_from_csr)

#define SS2_PORT_PML_CFG_GENERAL_CLOCK_PERIOD_PS_UPDATE(a, b)                        SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_general, clock_period_ps)

#define SS2_PORT_PML_CFG_LLR_CAPACITY_MAX_DATA_UPDATE(a, b)                          SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_capacity, max_data)
#define SS2_PORT_PML_CFG_LLR_CAPACITY_MAX_SEQ_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_capacity, max_seq)
#define SS2_PORT_PML_CFG_LLR_SIZE_UPDATE(a, b)                                       SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr, size)
#define SS2_PORT_PML_CFG_LLR_ACK_NACK_ERR_CHECK_UPDATE(a, b)                         SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr, ack_nack_err_check)
#define SS2_PORT_PML_CFG_LLR_PREAMBLE_SEQ_CHECK_UPDATE(a, b)                         SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr, preamble_seq_check)
#define SS2_PORT_PML_CFG_LLR_CF_RATES_LOOP_TIMING_PERIOD_UPDATE(a, b)                SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_cf_rates, loop_timing_period)
#define SS2_PORT_PML_CFG_LLR_CF_SMAC_CTL_FRAME_SMAC_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_cf_smac, ctl_frame_smac)
#define SS2_PORT_PML_CFG_LLR_CF_ETYPE_CTL_FRAME_ETHERTYPE_UPDATE(a, b)               SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_cf_etype, ctl_frame_ethertype)
#define SS2_PORT_PML_CFG_LLR_SM_REPLAY_CT_MAX_GET(a)                                 SL_CSR_GET(a, ss2_port_pml_cfg_llr_sm, replay_ct_max)
#define SS2_PORT_PML_CFG_LLR_SM_REPLAY_CT_MAX_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_sm, replay_ct_max)
#define SS2_PORT_PML_CFG_LLR_SM_REPLAY_TIMER_MAX_UPDATE(a, b)                        SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_sm, replay_timer_max)
#define SS2_PORT_PML_CFG_LLR_SM_RETRY_THRESHOLD_UPDATE(a, b)                         SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_sm, retry_threshold)
#define SS2_PORT_PML_CFG_LLR_SM_ALLOW_RE_INIT_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_sm, allow_re_init)
#define SS2_PORT_PML_CFG_LLR_SUBPORT_FILTER_LOSSLESS_WHEN_OFF_UPDATE(a, b)           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_subport, filter_lossless_when_off)
#define SS2_PORT_PML_CFG_LLR_SUBPORT_MAX_STARVATION_LIMIT_UPDATE(a, b)               SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_subport, max_starvation_limit)
#define SS2_PORT_PML_CFG_LLR_SUBPORT_LINK_DOWN_BEHAVIOR_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_subport, link_down_behavior)
#define SS2_PORT_PML_CFG_LLR_SUBPORT_MAC_IF_CREDITS_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_subport, mac_if_credits)
#define SS2_PORT_PML_CFG_LLR_SUBPORT_FILTER_CTL_FRAMES_UPDATE(a, b)                  SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_subport, filter_ctl_frames)
#define SS2_PORT_PML_CFG_LLR_SUBPORT_ENABLE_LOOP_TIMING_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_subport, enable_loop_timing)
#define SS2_PORT_PML_CFG_LLR_SUBPORT_LLR_MODE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_subport, llr_mode)
#define SS2_PORT_PML_CFG_LLR_TIMEOUTS_DATA_AGE_TIMER_MAX_UPDATE(a, b)                SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_timeouts, data_age_timer_max)
#define SS2_PORT_PML_CFG_LLR_TIMEOUTS_PCS_LINK_DN_TIMER_MAX_UPDATE(a, b)             SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_llr_timeouts, pcs_link_dn_timer_max)

#define SS2_PORT_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_GET(a)                         SL_CSR_GET(a, ss2_port_pml_cfg_pcs, enable_auto_lane_degrade)

#define SS2_PORT_PML_CFG_PCS_AUTONEG_RESET_UPDATE(a, b)                              SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs_autoneg, reset)
#define SS2_PORT_PML_CFG_PCS_AUTONEG_RESTART_UPDATE(a, b)                            SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs_autoneg, restart)
#define SS2_PORT_PML_CFG_PCS_AUTONEG_NEXT_PAGE_LOADED_UPDATE(a, b)                   SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs_autoneg, next_page_loaded)
#define SS2_PORT_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_UPDATE(a, b)                   SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs, enable_auto_lane_degrade)
#define SS2_PORT_PML_CFG_PCS_PCS_MODE_UPDATE(a, b)                                   SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs, pcs_mode)
#define SS2_PORT_PML_CFG_PCS_TIMESTAMP_SHIFT_UPDATE(a, b)                            SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs, timestamp_shift)
#define SS2_PORT_PML_CFG_PCS_SUBPORT_PCS_ENABLE_UPDATE(a, b)                         SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs_subport, pcs_enable)
#define SS2_PORT_PML_CFG_PCS_SUBPORT_ENABLE_AUTO_NEG_UPDATE(a, b)                    SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_pcs_subport, enable_auto_neg)

#define SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(a, b)                   SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs_subport, enable_ctl_os)
#define SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_LOCK_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs_subport, enable_lock)
#define SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_RX_SM_UPDATE(a, b)                    SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs_subport, enable_rx_sm)
#define SS2_PORT_PML_CFG_RX_PCS_SUBPORT_RS_MODE_UPDATE(a, b)                         SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs_subport, rs_mode)
#define SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_GET(a)                                  SL_CSR_GET(a, ss2_port_pml_cfg_rx_pcs, active_lanes)
#define SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(a, b)                            SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, active_lanes)
#define SS2_PORT_PML_CFG_RX_PCS_CW_GAP_544_UPDATE(a, b)                              SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, cw_gap_544)
#define SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_AMS_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, restart_lock_on_bad_ams)
#define SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, restart_lock_on_bad_cws)
#define SS2_PORT_PML_CFG_RX_PCS_ALLOW_AUTO_DEGRADE_UPDATE(a, b)                      SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, allow_auto_degrade)
#define SS2_PORT_PML_CFG_RX_PCS_LANE_0_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, lane_0_source)
#define SS2_PORT_PML_CFG_RX_PCS_LANE_1_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, lane_1_source)
#define SS2_PORT_PML_CFG_RX_PCS_LANE_2_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, lane_2_source)
#define SS2_PORT_PML_CFG_RX_PCS_LANE_3_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_pcs, lane_3_source)

#define SS2_PORT_PML_CFG_TX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(a, b)                   SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs_subport, enable_ctl_os)
#define SS2_PORT_PML_CFG_TX_PCS_CDC_READY_LEVEL_UPDATE(a, b)                         SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs, cdc_ready_level)
#define SS2_PORT_PML_CFG_TX_PCS_EN_PK_BW_LIMITER_UPDATE(a, b)                        SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs, en_pk_bw_limiter)
#define SS2_PORT_PML_CFG_TX_PCS_LANE_0_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs, lane_0_source)
#define SS2_PORT_PML_CFG_TX_PCS_LANE_1_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs, lane_1_source)
#define SS2_PORT_PML_CFG_TX_PCS_LANE_2_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs, lane_2_source)
#define SS2_PORT_PML_CFG_TX_PCS_LANE_3_SOURCE_UPDATE(a, b)                           SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs, lane_3_source)
#define SS2_PORT_PML_CFG_TX_PCS_ALLOW_AUTO_DEGRADE_UPDATE(a, b)                      SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs, allow_auto_degrade)
#define SS2_PORT_PML_CFG_TX_PCS_SUBPORT_GEARBOX_CREDITS_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_pcs_subport, gearbox_credits)

#define SS2_PORT_PML_CFG_RX_MAC_FLIT_PACKING_CNT_UPDATE(a, b)                        SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_mac, flit_packing_cnt)
#define SS2_PORT_PML_CFG_RX_MAC_SUBPORT_MAC_OPERATIONAL_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_mac_subport, mac_operational)
#define SS2_PORT_PML_CFG_RX_MAC_SUBPORT_MAC_OPERATIONAL_GET(a)                       SL_CSR_GET(a, ss2_port_pml_cfg_rx_mac_subport, mac_operational)
#define SS2_PORT_PML_CFG_RX_MAC_SUBPORT_SHORT_PREAMBLE_UPDATE(a, b)                  SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_rx_mac_subport, short_preamble)

#define SS2_PORT_PML_CFG_TX_MAC_IEEE_IFG_ADJUSTMENT_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac, ieee_ifg_adjustment)
#define SS2_PORT_PML_CFG_TX_MAC_MAC_PAD_IDLE_THRESH_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac, mac_pad_idle_thresh)
#define SS2_PORT_PML_CFG_TX_MAC_IFG_MODE_UPDATE(a, b)                                SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac, ifg_mode)
#define SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_OPERATIONAL_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac_subport, mac_operational)
#define SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_OPERATIONAL_GET(a)                       SL_CSR_GET(a, ss2_port_pml_cfg_tx_mac_subport, mac_operational)
#define SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_THRESH_UPDATE(a, b)                  SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac_subport, mac_cdt_thresh)
#define SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_INIT_VAL_UPDATE(a, b)                SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac_subport, mac_cdt_init_val)
#define SS2_PORT_PML_CFG_TX_MAC_SUBPORT_PCS_CREDITS_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac_subport, pcs_credits)
#define SS2_PORT_PML_CFG_TX_MAC_SUBPORT_SHORT_PREAMBLE_UPDATE(a, b)                  SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_tx_mac_subport, short_preamble)

#define SS2_PORT_PML_CFG_SERDES_RX_PMD_RX_LANE_MODE_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_rx, pmd_rx_lane_mode)
#define SS2_PORT_PML_CFG_SERDES_RX_PMD_RX_OSR_MODE_GET(a)                            SL_CSR_GET(a, ss2_port_pml_cfg_serdes_rx, pmd_rx_osr_mode)
#define SS2_PORT_PML_CFG_SERDES_RX_PMD_RX_OSR_MODE_UPDATE(a, b)                      SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_rx, pmd_rx_osr_mode)
#define SS2_PORT_PML_CFG_SERDES_RX_PMD_EXT_LOS_UPDATE(a, b)                          SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_rx, pmd_ext_los)
#define SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_H_PWRDN_UPDATE(a, b)                    SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_rx, pmd_ln_rx_h_pwrdn)
#define SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_DP_H_RSTB_UPDATE(a, b)                  SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_rx, pmd_ln_rx_dp_h_rstb)
#define SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_H_RSTB_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_rx, pmd_ln_rx_h_rstb)

#define SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_LANE_MODE_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_tx, pmd_tx_lane_mode)
#define SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_UPDATE(a, b)                      SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_tx, pmd_tx_osr_mode)
#define SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_DISABLE_UPDATE(a, b)                       SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_tx, pmd_tx_disable)
#define SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_H_PWRDN_UPDATE(a, b)                    SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_tx, pmd_ln_tx_h_pwrdn)
#define SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_DP_H_RSTB_UPDATE(a, b)                  SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_tx, pmd_ln_tx_dp_h_rstb)
#define SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_H_RSTB_UPDATE(a, b)                     SL_CSR_UPDATE(a, b, ss2_port_pml_cfg_serdes_tx, pmd_ln_tx_h_rstb)
#define SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_GET(a)                            SL_CSR_GET(a, ss2_port_pml_cfg_serdes_tx, pmd_tx_osr_mode)

#define SS2_PORT_PML_STS_LLR_LOOP_TIME_LOOP_TIME_GET(a)                              SL_CSR_GET(a, ss2_port_pml_sts_llr_loop_time, loop_time)
#define SS2_PORT_PML_STS_LLR_LLR_STATE_GET(a)                                        SL_CSR_GET(a, ss2_port_pml_sts_llr, llr_state)

#define SS2_PORT_PML_STS_RX_PCS_SUBPORT_ALIGN_STATUS_GET(a)                          SL_CSR_GET(a, ss2_port_pml_sts_rx_pcs_subport, align_status)
#define SS2_PORT_PML_STS_RX_PCS_SUBPORT_FAULT_GET(a)                                 SL_CSR_GET(a, ss2_port_pml_sts_rx_pcs_subport, fault)
#define SS2_PORT_PML_STS_RX_PCS_SUBPORT_LOCAL_FAULT_GET(a)                           SL_CSR_GET(a, ss2_port_pml_sts_rx_pcs_subport, local_fault)
#define SS2_PORT_PML_STS_RX_PCS_SUBPORT_HI_SER_GET(a)                                SL_CSR_GET(a, ss2_port_pml_sts_rx_pcs_subport, hi_ser)

#define SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_BASE_PAGE_GET(a)                   SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_base_page, lp_base_page)
#define SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_ABILITY_GET(a)                     SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_base_page, lp_ability)
#define SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_BASE_PAGE_GET(a)                      SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_base_page, base_page)
#define SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_COMPLETE_GET(a)                       SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_base_page, complete)
#define SS2_PORT_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(a)                          SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_base_page, state)
#define SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_NEXT_PAGE_GET(a)                   SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_next_page, lp_next_page)
#define SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_ABILITY_GET(a)                     SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_next_page, lp_ability)
#define SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_BASE_PAGE_GET(a)                      SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_next_page, base_page)
#define SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_COMPLETE_GET(a)                       SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_next_page, complete)
#define SS2_PORT_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(a)                          SL_CSR_GET(a, ss2_port_pml_sts_pcs_autoneg_next_page, state)

#define SS2_PORT_PML_STS_PCS_LANE_DEGRADE_WORD0_LP_PLS_AVAILABLE_GET(a)              SL_CSR_WORD_GET(a, ss2_port_pml_sts_pcs_lane_degrade, lp_pls_available, 0)
#define SS2_PORT_PML_STS_PCS_LANE_DEGRADE_WORD0_RX_PLS_AVAILABLE_GET(a)              SL_CSR_WORD_GET(a, ss2_port_pml_sts_pcs_lane_degrade, rx_pls_available, 0)

#define SS2_PORT_PML_STS_SERDES_PMD_TX_DATA_VLD_GET(a)                               SL_CSR_GET(a, ss2_port_pml_sts_serdes, pmd_tx_data_vld)
#define SS2_PORT_PML_STS_SERDES_PMD_TX_CLK_VLD_GET(a)                                SL_CSR_GET(a, ss2_port_pml_sts_serdes, pmd_tx_clk_vld)
#define SS2_PORT_PML_STS_SERDES_PMD_RX_DATA_VLD_GET(a)                               SL_CSR_GET(a, ss2_port_pml_sts_serdes, pmd_rx_data_vld)
#define SS2_PORT_PML_STS_SERDES_PMD_RX_CLK_VLD_GET(a)                                SL_CSR_GET(a, ss2_port_pml_sts_serdes, pmd_rx_clk_vld)
#define SS2_PORT_PML_STS_SERDES_PMD_RX_LOCK_GET(a)                                   SL_CSR_GET(a, ss2_port_pml_sts_serdes, pmd_rx_lock)
#define SS2_PORT_PML_STS_SERDES_PMD_SIGNAL_DETECTGET(a)                              SL_CSR_GET(a, ss2_port_pml_sts_serdes, pmd_signal_detect)

#define SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_DISCARD                                   C2_HNI_PML_LD_DISCARD
#define SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_BLOCK                                     C2_HNI_PML_LD_BLOCK
#define SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_BEST_EFFORT                               C2_HNI_PML_LD_BEST_EFFORT

#define SS2_PORT_PML_ERR_INFO_PCS_TX_DP_TX_CDC_UNDERRUN_UPDATE(a, b)                 SL_CSR_UPDATE(a, b, ss2_port_pml_err_info_pcs_tx_dp, tx_cdc_underrun)

#define SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_0_GET(a)                        SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, llr_replay_at_max_0, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_1_GET(a)                        SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, llr_replay_at_max_1, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_2_GET(a)                        SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, llr_replay_at_max_2, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_3_GET(a)                        SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, llr_replay_at_max_3, 1)

#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_0_GET(a)                            SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_0, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_1_GET(a)                            SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_1, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_2_GET(a)                            SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_2, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_3_GET(a)                            SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_3, 1)

#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_0_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_lf_0, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_1_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_lf_1, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_2_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_lf_2, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_3_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_lf_3, 1)

#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_0_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_rf_0, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_1_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_rf_1, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_2_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_rf_2, 1)
#define SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_3_GET(a)                         SL_CSR_WORD_GET(a, ss2_port_pml_err_flg, pcs_link_down_rf_3, 1)

#endif /* _SL_ASIC_CSRS_H_ */
