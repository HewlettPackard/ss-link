// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_core_str.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_llr.h"
#include "base/sl_core_work_llr.h"
#include "base/sl_core_timer_llr.h"
#include "base/sl_core_log.h"
#include "data/sl_core_data_lgrp.h"
#include "data/sl_core_data_llr.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_llr.h"

#define SL_CORE_LLR_BYTES_PER_FRAME    9216ULL
#define SL_CORE_LLR_NUM_FRAMES         2ULL
#define SL_CORE_LLR_BYTE_QUANTA        64ULL
#define SL_CORE_LLR_FRAME_SIZE         32ULL
#define SL_CORE_LLR_ASIC_TX_DELAY_MS   25
#define SL_CORE_LLR_ASIC_RX_DELAY_MS   91

#define SL_CORE_LLR_STOP_TIMEOUT       2000

#define LOG_NAME SL_CORE_HW_LLR_LOG_NAME

void sl_core_hw_llr_link_init(struct sl_core_link *core_link)
{
	u32 port;
	u64 data64;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "link init (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_LLR_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_LINK_DOWN_BEHAVIOR_UPDATE(data64,
		SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_DISCARD);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_LLR_SUBPORT(core_link->num), data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_LLR_SUBPORT(core_link->num));
}

static void sl_core_hw_llr_setup_callback(struct sl_core_llr *core_llr)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "setup callback");

	core_llr->callbacks.setup(core_llr->tag, core_llr->state,
		core_llr->info_map, core_llr->data);
}

static void sl_core_hw_llr_start_callback(struct sl_core_llr *core_llr)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "start callback");

	core_llr->callbacks.start(core_llr->tag, core_llr->state, core_llr->info_map);
}

static void sl_core_hw_llr_loop_time_start(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "loop time start (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_ENABLE_LOOP_TIMING_UPDATE(data64, 1); /* enable */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num));
}

static void sl_core_hw_llr_loop_time_stop(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "loop time stop (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_ENABLE_LOOP_TIMING_UPDATE(data64, 0); /* disable */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num));
}

static void sl_core_hw_llr_ordered_sets_start(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "ordered sets start (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(data64, 1); /* enable */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_llr->num), data64);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_TX_PCS_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_TX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(data64, 1); /* enable */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_TX_PCS_SUBPORT(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num));
}

static void sl_core_hw_llr_ordered_sets_stop(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "ordered sets stop (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(data64, 0); /* disable */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_llr->num), data64);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_TX_PCS_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_TX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(data64, 0); /* disable */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_TX_PCS_SUBPORT(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num));
}

static void sl_core_hw_llr_on(struct sl_core_llr *core_llr)
{
	u32                  port;
	u64                  data64;
	struct sl_core_link *core_link;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "on (port = %d)", port);

	core_link = sl_core_link_get(core_llr->core_lgrp->core_ldev->num,
		core_llr->core_lgrp->num, core_llr->num);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_RX_PCS, &data64);
	/* NOTE: these need to match the restart_lock settings on the link */
	if (sl_core_link_config_is_enable_ald_set(core_link))
		data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(data64, 0);
	else
		data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(data64, 1);

	data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_AMS_UPDATE(data64, 1);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_RX_PCS, data64);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_LLR_MODE_UPDATE(data64, 2); /* ON */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num));
}

static void sl_core_hw_llr_off(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "off (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_LLR_MODE_UPDATE(data64, 0); /* OFF */
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num));
}

static void sl_core_hw_llr_discard(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "discard (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_LINK_DOWN_BEHAVIOR_UPDATE(data64,
		SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_DISCARD);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num));
}

#define SL_CORE_PACKET_BYTES_MAX 9000
static void sl_core_hw_llr_capacity_set(struct sl_core_llr *core_llr)
{
	u32                port;
	u64                bytes;
	u64                calc_data;
	int                x;
	u64                total_time;
	u64                data64;
	struct sl_llr_data llr_data;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "capacity set (port = %d)", port);

	llr_data.loop.min = core_llr->loop_time[0];
	llr_data.loop.max = core_llr->loop_time[0];
	total_time        = core_llr->loop_time[0];
	for (x = 1; x < SL_CORE_LLR_MAX_LOOP_TIME_COUNT; ++x) {
		llr_data.loop.min  = min(core_llr->loop_time[x], llr_data.loop.min);
		llr_data.loop.max  = max(core_llr->loop_time[x], llr_data.loop.max);
		total_time        += core_llr->loop_time[x];
	}
	llr_data.loop.average = DIV_ROUND_UP(total_time, SL_CORE_LLR_MAX_LOOP_TIME_COUNT);

	if (core_llr->core_lgrp->config.options & SL_LGRP_CONFIG_OPT_FABRIC) {
		calc_data = 0x800; /* reset value */
	} else {
		bytes = (llr_data.loop.average * core_llr->settings.bytes_per_ns) +
			(SL_CORE_LLR_BYTES_PER_FRAME * SL_CORE_LLR_NUM_FRAMES) +
			SL_CORE_PACKET_BYTES_MAX;

		sl_core_log_dbg(core_llr, LOG_NAME,
			"capacity set (average = %lldns, byte_per_ns = %d, bytes = %lld)",
			llr_data.loop.average, core_llr->settings.bytes_per_ns, bytes);

		calc_data = DIV_ROUND_UP(bytes, SL_CORE_LLR_BYTE_QUANTA);
		if (calc_data > core_llr->settings.max_cap_data)
			calc_data = core_llr->settings.max_cap_data;
	}
	sl_core_log_dbg(core_llr, LOG_NAME, "capacity set (data = %lld)", calc_data);

	core_llr->settings.replay_ct_max    = 0xFE;
	core_llr->settings.replay_timer_max = (3 * llr_data.loop.average + 500);

	if (core_llr->settings.replay_timer_max < 1000)
		core_llr->settings.replay_timer_max = 1000;

	if (core_llr->settings.replay_timer_max > 15500)
		core_llr->settings.replay_timer_max = 15500;

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_CAPACITY(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_CAPACITY_MAX_DATA_UPDATE(data64, calc_data);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_CAPACITY(core_llr->num), data64);

	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_CAPACITY(core_llr->num));

	// FIXME: temp
	{
	u64 len  = 100;
	u64 rate = 4;
	// FIXME: add other cables
	llr_data.loop.calculated  = 2 * len * rate / 100;
	llr_data.loop.calculated += 2 * (SL_CORE_LLR_ASIC_TX_DELAY_MS + SL_CORE_LLR_ASIC_RX_DELAY_MS);
	}

	sl_core_data_llr_data_set(core_llr, llr_data);
}

static void sl_core_hw_llr_config(struct sl_core_llr *core_llr)
{
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "config (port = %d)", port);

	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_CONFIG);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR, &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SIZE_UPDATE(data64,
		core_llr->settings.size);
	data64 = SS2_PORT_PML_CFG_LLR_ACK_NACK_ERR_CHECK_UPDATE(data64, 1);
	data64 = SS2_PORT_PML_CFG_LLR_PREAMBLE_SEQ_CHECK_UPDATE(data64, 1);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR, data64);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_FILTER_LOSSLESS_WHEN_OFF_UPDATE(data64,
		core_llr->settings.lossless_when_off);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_LINK_DOWN_BEHAVIOR_UPDATE(data64,
		core_llr->settings.link_down_behavior);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_FILTER_CTL_FRAMES_UPDATE(data64,
		core_llr->settings.filter_ctl_frames);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_ENABLE_LOOP_TIMING_UPDATE(data64, 0);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_LLR_MODE_UPDATE(data64, 0);
	// FIXME: Setting to REPLAY_TIMER_MAX reset value for now. Need to get better # from SV.
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_MAX_STARVATION_LIMIT_UPDATE(data64, 1550);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SUBPORT(core_llr->num), data64);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_CF_SMAC, &data64);
	data64 = SS2_PORT_PML_CFG_LLR_CF_SMAC_CTL_FRAME_SMAC_UPDATE(data64,
		core_llr->settings.ctl_frame_smac);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_CF_SMAC, data64);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_CF_ETYPE, &data64);
	data64 = SS2_PORT_PML_CFG_LLR_CF_ETYPE_CTL_FRAME_ETHERTYPE_UPDATE(data64,
		core_llr->settings.ctl_frame_ethertype);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_CF_ETYPE, data64);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SM_RETRY_THRESHOLD_UPDATE(data64,
		core_llr->settings.retry_threshold);
	data64 = SS2_PORT_PML_CFG_LLR_SM_ALLOW_RE_INIT_UPDATE(data64,
		core_llr->settings.allow_re_init);
	data64 = SS2_PORT_PML_CFG_LLR_SM_REPLAY_CT_MAX_UPDATE(data64,
		core_llr->settings.replay_ct_max);
	data64 = SS2_PORT_PML_CFG_LLR_SM_REPLAY_TIMER_MAX_UPDATE(data64,
		core_llr->settings.replay_timer_max);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num), data64);

#ifdef BUILDSYS_FRAMEWORK_ROSETTA
	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_0(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_0_PCS_LINK_DN_TIMER_MAX_UPDATE(data64, 0x389ACA00);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_0(core_llr->num), data64);
	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_1(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_1_DATA_AGE_TIMER_MAX_UPDATE(data64, 0xEE6B2800);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_1(core_llr->num), data64);
	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS_1(core_llr->num));
#else /* Cassini */
	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_DATA_AGE_TIMER_MAX_UPDATE(data64, 0xEE6B2800);
	data64 = SS2_PORT_PML_CFG_LLR_TIMEOUTS_PCS_LINK_DN_TIMER_MAX_UPDATE(data64, 0x389ACA00);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS(core_llr->num), data64);
	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_TIMEOUTS(core_llr->num));
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */
}

//---------------------------- SETUP

void sl_core_hw_llr_setup_cmd(struct sl_core_llr *core_llr,
	sl_core_llr_setup_callback_t callback, void *tag, u32 flags)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "setup cmd (core_llr = 0x%p)", core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_CONFIG);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);
	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_REPLAY_MAX);

	core_llr->tag             = tag;
	core_llr->callbacks.setup = callback;
	core_llr->flags.setup     = flags;

	sl_core_timer_llr_begin(core_llr, SL_CORE_TIMER_LLR_SETUP);

	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"setup cmd llr_flgs_disable failed [%d]", rtn);

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);
	sl_core_hw_llr_config(core_llr);

	queue_work(core_llr->core_lgrp->core_ldev->workqueue, &(core_llr->work[SL_CORE_WORK_LLR_SETUP]));
}

static void sl_core_hw_llr_setup_fail(struct sl_core_llr *core_llr)
{
	u32 llr_state;

	sl_core_log_dbg(core_llr, LOG_NAME, "setup fail");

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_SETTING_UP:
		sl_core_log_dbg(core_llr, LOG_NAME, "setup fail - canceling");
		core_llr->state = SL_CORE_LLR_STATE_SETUP_CANCELING;
		spin_unlock(&core_llr->data_lock);
		sl_core_hw_llr_settingup_cancel_cmd(core_llr);
		return;
	default:
		sl_core_log_err(core_llr, LOG_NAME, "setup fail - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return;
	}
}

void sl_core_hw_llr_setup_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 llr_state;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP]);

	sl_core_log_dbg(core_llr, LOG_NAME, "setup work");

	llr_state = sl_core_data_llr_state_get(core_llr);
	if (llr_state != SL_CORE_LLR_STATE_SETTING_UP) {
		sl_core_log_dbg(core_llr, LOG_NAME, "setup work - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		return;
	}

	sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);
	sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);

	sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	rtn = sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0) {
		sl_core_log_err_trace(core_llr, LOG_NAME,
			"setup work llr_flgs_enable failed [%d]", rtn);
		sl_core_data_llr_last_fail_cause_set(core_llr, SL_LLR_FAIL_CAUSE_SETUP_INTR_ENABLE);
		sl_core_hw_llr_setup_fail(core_llr);
		return;
	}

	sl_core_hw_llr_ordered_sets_start(core_llr);
	sl_core_hw_llr_loop_time_start(core_llr);
}

void sl_core_hw_llr_setup_loop_time_intr_work(struct work_struct *work)
{
	u32                 port;
	int                 x;
	u64                 data64;
	u64                 loop_time;
	struct sl_core_llr *core_llr;
	int                 tries;
	u32                 llr_state;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP_LOOP_TIME_INTR]);

	llr_state = sl_core_data_llr_state_get(core_llr);
	if (llr_state != SL_CORE_LLR_STATE_SETTING_UP) {
		sl_core_log_dbg(core_llr, LOG_NAME, "loop time intr work - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		return;
	}

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "loop time intr work (port = %d)", port);

	x = 0;
	tries = 0;
	while (x < SL_CORE_LLR_MAX_LOOP_TIME_COUNT) {
		if (sl_core_llr_setup_should_stop(core_llr)) {
			sl_core_log_dbg(core_llr, LOG_NAME, "loop time intr work canceled");
			return;
		}

		sl_core_llr_write64(core_llr, SS2_PORT_PML_STS_LLR_LOOP_TIME(core_llr->num), 0);
		sl_core_llr_flush64(core_llr, SS2_PORT_PML_STS_LLR_LOOP_TIME(core_llr->num));
		udelay(50);
		sl_core_llr_read64(core_llr, SS2_PORT_PML_STS_LLR_LOOP_TIME(core_llr->num), &data64);
		loop_time = SS2_PORT_PML_STS_LLR_LOOP_TIME_LOOP_TIME_GET(data64);

		sl_core_log_dbg(core_llr, LOG_NAME,
			"loop time intr work (time %2d = %lluns)", x, loop_time);

		if (tries++ > 100)
			break;

		if (loop_time == 0)
			continue;

		core_llr->loop_time[x] = loop_time;

		++x;
	}

	sl_core_hw_llr_capacity_set(core_llr);

	sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_SETUP);

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_SETTING_UP:
		sl_core_log_dbg(core_llr, LOG_NAME, "setup loop time - setup");
		core_llr->state = SL_CORE_LLR_STATE_SETUP;
		clear_bit(SL_CORE_INFO_MAP_LLR_SETTING_UP, (unsigned long *)&(core_llr->info_map));
		set_bit(SL_CORE_INFO_MAP_LLR_SETUP, (unsigned long *)&(core_llr->info_map));
		spin_unlock(&core_llr->data_lock);

		sl_core_hw_llr_setup_callback(core_llr);

		return;
	default:
		sl_core_log_err(core_llr, LOG_NAME, "setup loop time - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return;
	}
}

void sl_core_hw_llr_setup_unexp_loop_time_intr_work(struct work_struct *work)
{
	struct sl_core_llr *core_llr;
	u32                 llr_state;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP_UNEXP_LOOP_TIME_INTR]);

	sl_core_log_dbg(core_llr, LOG_NAME, "unexpected loop time received");

	llr_state = sl_core_data_llr_state_get(core_llr);
	if (llr_state != SL_CORE_LLR_STATE_SETTING_UP) {
		sl_core_log_dbg(core_llr, LOG_NAME, "unexpected loop time intr work - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		return;
	}
}

void sl_core_hw_llr_setup_timeout_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 llr_state;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP_TIMEOUT]);

	sl_core_log_dbg(core_llr, LOG_NAME, "setup timeout work");

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_SETTING_UP:
		core_llr->state = SL_CORE_LLR_STATE_SETUP_TIMEOUT;
		spin_unlock(&core_llr->data_lock);
		break;
	default:
		sl_core_log_err(core_llr, LOG_NAME, "start timeout work - invalid state (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return;
	}

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);

	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"setup timeout work llr_flgs_disable failed [%d]", rtn);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT);

	sl_core_data_llr_last_fail_cause_set(core_llr, SL_LLR_FAIL_CAUSE_SETUP_TIMEOUT);
	sl_core_hw_llr_setup_callback(core_llr);
	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_CONFIGURED);

	complete_all(&core_llr->stop_complete);
}

void sl_core_hw_llr_settingup_cancel_cmd(struct sl_core_llr *core_llr)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "settingup cancel cmd");

	sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_SETUP);

	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_TIMEOUT]));

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"settingup cancel cmd llr_flgs_disable failed [%d]", rtn);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"settingup cancel cmd llr_flgs_disable failed [%d]", rtn);


	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_LOOP_TIME_INTR]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_UNEXP_LOOP_TIME_INTR]));

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_CONFIGURED);

	complete_all(&core_llr->stop_complete);

	sl_core_hw_llr_setup_callback(core_llr);
}

//---------------------------- START

void sl_core_hw_llr_start_cmd(struct sl_core_llr *core_llr,
	sl_core_llr_start_callback_t callback, void *tag, u32 flags)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "start cmd");

	sl_core_data_llr_last_fail_cause_set(core_llr, SL_LLR_FAIL_CAUSE_NONE);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);
	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);

	core_llr->tag             = tag;
	core_llr->callbacks.start = callback;
	core_llr->flags.start     = flags;

	sl_core_timer_llr_begin(core_llr, SL_CORE_TIMER_LLR_START);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"start cmd llr_flgs_disable failed [%d]", rtn);

	queue_work(core_llr->core_lgrp->core_ldev->workqueue, &(core_llr->work[SL_CORE_WORK_LLR_START]));
}

void sl_core_hw_llr_start_fail(struct sl_core_llr *core_llr)
{
	u32 llr_state;

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_STARTING:
		sl_core_log_dbg(core_llr, LOG_NAME, "start fail - canceling");
		core_llr->state = SL_CORE_LLR_STATE_START_CANCELING;
		clear_bit(SL_CORE_INFO_MAP_LLR_STARTING, (unsigned long *)&(core_llr->info_map));
		spin_unlock(&core_llr->data_lock);
		sl_core_hw_llr_starting_cancel_cmd(core_llr);
		return;
	default:
		sl_core_log_err(core_llr, LOG_NAME, "start fail - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return;
	}
}

void sl_core_hw_llr_start_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 llr_state;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_START]);

	sl_core_log_dbg(core_llr, LOG_NAME, "start work");

	llr_state = sl_core_data_llr_state_get(core_llr);
	if (llr_state != SL_CORE_LLR_STATE_STARTING) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start work - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		return;
	}

	sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	rtn = sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0) {
		sl_core_log_err_trace(core_llr, LOG_NAME,
			"start work llr_flgs_enable failed [%d]", rtn);
		sl_core_data_llr_last_fail_cause_set(core_llr, SL_LLR_FAIL_CAUSE_START_INTR_ENABLE);
		sl_core_hw_llr_start_fail(core_llr);
		return;
	}

	sl_core_hw_llr_on(core_llr);
}

void sl_core_hw_llr_start_init_complete_intr_work(struct work_struct *work)
{
	u32                 port;
	u32                 llr_state;
	u64                 data64;
	struct sl_core_llr *core_llr;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR]);

	llr_state = sl_core_data_llr_state_get(core_llr);
	if (llr_state != SL_CORE_LLR_STATE_STARTING) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start init complete intr work - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		return;
	}

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "start init complete intr work (port = %d)", port);

	if (sl_core_llr_start_should_stop(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start init complete intr work canceled");
		return;
	}

	sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_START);

	sl_core_hw_llr_loop_time_stop(core_llr);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SM_REPLAY_CT_MAX_UPDATE(data64,
		core_llr->settings.replay_ct_max);
	data64 = SS2_PORT_PML_CFG_LLR_SM_REPLAY_TIMER_MAX_UPDATE(data64,
		core_llr->settings.replay_timer_max);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num), data64);
	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num));

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_STARTING:
		sl_core_log_dbg(core_llr, LOG_NAME, "start init complete - running");
		core_llr->state = SL_CORE_LLR_STATE_RUNNING;
		set_bit(SL_CORE_INFO_MAP_LLR_RUNNING, (unsigned long *)&(core_llr->info_map));
		clear_bit(SL_CORE_INFO_MAP_LLR_STARTING, (unsigned long *)&(core_llr->info_map));
		spin_unlock(&core_llr->data_lock);

		sl_core_hw_llr_start_callback(core_llr);
		return;
	default:
		sl_core_log_err(core_llr, LOG_NAME, "start init complete - invalid state (%u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return;
	}
}

void sl_core_hw_llr_start_timeout_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 llr_state;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_START_TIMEOUT]);

	sl_core_log_dbg(core_llr, LOG_NAME, "start timeout work");

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_STARTING:
		core_llr->state = SL_CORE_LLR_STATE_START_TIMEOUT;
		spin_unlock(&core_llr->data_lock);
		break;
	default:
		sl_core_log_err(core_llr, LOG_NAME, "start timeout work - invalid state (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return;
	}

	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START]));

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"start timeout work llr_flgs_disable failed [%d]", rtn);

	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR]));

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);
	sl_core_hw_llr_off(core_llr);
	sl_core_hw_llr_discard(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);

	sl_core_data_llr_last_fail_cause_set(core_llr, SL_LLR_FAIL_CAUSE_START_TIMEOUT);
	sl_core_hw_llr_start_callback(core_llr);
	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_SETUP);

	complete_all(&core_llr->stop_complete);
}

void sl_core_hw_llr_starting_cancel_cmd(struct sl_core_llr *core_llr)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "starting cancel cmd");

	sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_START);

	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START_TIMEOUT]));

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0)
		sl_core_log_warn_trace(core_llr, LOG_NAME,
			"starting cancel cmd llr_flgs_disable failed [%d]", rtn);

	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR]));

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);
	sl_core_hw_llr_off(core_llr);
	sl_core_hw_llr_discard(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_SETUP);

	complete_all(&core_llr->stop_complete);

	sl_core_hw_llr_start_callback(core_llr);
}

//---------------------------- STOP

void sl_core_hw_llr_stop(struct sl_core_llr *core_llr)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "stop");

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);
	sl_core_hw_llr_off(core_llr);
	sl_core_hw_llr_discard(core_llr);
}

void sl_core_hw_llr_setup_stop_cmd(struct sl_core_llr *core_llr)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "setup stop cmd");

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);

	sl_core_data_llr_data_clr(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_REPLAY_MAX);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_CONFIGURED);

	complete_all(&core_llr->stop_complete);
}

void sl_core_hw_llr_running_stop_cmd(struct sl_core_llr *core_llr)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "running stop cmd");

	sl_core_hw_llr_off(core_llr);
	sl_core_hw_llr_discard(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_REPLAY_MAX);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_SETUP);

	complete_all(&core_llr->stop_complete);
}

int sl_core_hw_llr_stop_wait(struct sl_core_llr *core_llr)
{
	unsigned long timeleft;

	sl_core_log_dbg(core_llr, LOG_NAME, "stop wait");

	timeleft = wait_for_completion_timeout(&core_llr->stop_complete,
		msecs_to_jiffies(SL_CORE_LLR_STOP_TIMEOUT));
	if (timeleft == 0) {
		sl_core_log_err(core_llr, LOG_NAME,
			"completion_timeout (stop_complete = 0x%p, timeleft = %lu)",
			&core_llr->stop_complete, timeleft);
		return -ETIMEDOUT;
	}

	return 0;
}
