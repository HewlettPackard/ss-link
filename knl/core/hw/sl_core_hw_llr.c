// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>

#include "sl_kconfig.h"
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
	int                 rtn;
	struct sl_llr_data *llr_data;

	sl_core_log_dbg(core_llr, LOG_NAME, "setup callback");

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "setup callback canceled");
		return;
	}

	llr_data = kmem_cache_alloc(core_llr->data_cache, GFP_ATOMIC);
	if (llr_data == NULL) {
		sl_core_log_err(core_llr, LOG_NAME,
			"setup callback cache alloc failed");
		return;
	}
	*llr_data = sl_core_data_llr_data_get(core_llr);

	rtn = core_llr->callbacks.setup(core_llr->tag,
		core_llr->state, sl_core_data_llr_info_map_get(core_llr), llr_data);
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"setup callback failed [%d]", rtn);
}

static void sl_core_hw_llr_start_callback(struct sl_core_llr *core_llr)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "start callback");

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start callback canceled");
		return;
	}

	rtn = core_llr->callbacks.start(core_llr->tag,
		core_llr->state, sl_core_data_llr_info_map_get(core_llr));
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"start callback failed [%d]", rtn);
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
	u32 port;
	u64 data64;

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "on (port = %d)", port);

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_RX_PCS, &data64);
	/* NOTE: these need to match the restart_lock settings on the link */
	data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(data64, 0);
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

	if (core_llr->core_lgrp->config.options & SL_LGRP_OPT_FABRIC) {
		calc_data = 0x800; /* reset value */
	} else {
		bytes = (llr_data.loop.average * core_llr->settings.bytes_per_ns) +
			(SL_CORE_LLR_BYTES_PER_FRAME * SL_CORE_LLR_NUM_FRAMES);

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
	sl_core_llr_is_canceled_clr(core_llr);

	sl_core_timer_llr_begin(core_llr, SL_CORE_TIMER_LLR_SETUP);

	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"setup cmd llr_flgs_disable failed [%d]", rtn);

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);
	sl_core_hw_llr_off(core_llr);
	sl_core_hw_llr_config(core_llr);

	if (sl_core_data_llr_data_is_valid(core_llr) &&
		(is_flag_set(core_llr->flags.setup, SL_CORE_LLR_FLAG_SETUP_REUSE_TIMING)))
		sl_core_work_llr_queue(core_llr, SL_CORE_WORK_LLR_SETUP_REUSE_TIMING);
	else
		sl_core_work_llr_queue(core_llr, SL_CORE_WORK_LLR_SETUP);
}

void sl_core_hw_llr_setup_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP]);

	sl_core_log_dbg(core_llr, LOG_NAME, "setup work");

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "setup work canceled");
		return;
	}

	sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);
	sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);

	sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	rtn = sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0) {
		sl_core_log_err(core_llr, LOG_NAME,
			"setup work llr_flgs_enable failed [%d]", rtn);
		rtn = sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_SETUP);
		if (rtn < 0)
			sl_core_log_warn(core_llr, LOG_NAME,
				"setup work timer_llr_end failed [%d]", rtn);
		sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
		sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_CONFIGURED);
		sl_core_hw_llr_setup_callback(core_llr);
		return;
	}

	sl_core_hw_llr_ordered_sets_start(core_llr);
	sl_core_hw_llr_loop_time_start(core_llr);
}

void sl_core_hw_llr_setup_reuse_timing_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP_REUSE_TIMING]);

	sl_core_log_dbg(core_llr, LOG_NAME, "setup reuse timing work");

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "setup reuse timing work canceled");
		return;
	}

	sl_core_hw_llr_capacity_set(core_llr);

	sl_core_hw_llr_ordered_sets_start(core_llr);

	rtn = sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_SETUP);
	if (rtn < 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"setup reuse timing work timer_llr_end failed [%d]", rtn);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_SETUP);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_SETUP);
	sl_core_hw_llr_setup_callback(core_llr);
}

void sl_core_hw_llr_setup_loop_time_intr_work(struct work_struct *work)
{
	int                 rtn;
	u32                 port;
	int                 x;
	u64                 data64;
	u64                 loop_time;
	struct sl_core_llr *core_llr;
	int                 tries;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP_LOOP_TIME_INTR]);

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "loop time intr work (port = %d)", port);

	x = 0;
	tries = 0;
	while (x < SL_CORE_LLR_MAX_LOOP_TIME_COUNT) {
		if (sl_core_llr_is_canceled(core_llr)) {
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

	rtn = sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_SETUP);
	if (rtn < 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"loop time intr work timer_llr_end failed [%d]", rtn);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_SETUP);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_SETUP);
	sl_core_hw_llr_setup_callback(core_llr);
}

void sl_core_hw_llr_setup_unexp_loop_time_intr_work(struct work_struct *work)
{
	struct sl_core_llr *core_llr;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP_UNEXP_LOOP_TIME_INTR]);

	sl_core_log_dbg(core_llr, LOG_NAME, "unexpected loop time received");
}

void sl_core_hw_llr_setup_timeout_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_SETUP_TIMEOUT]);

	sl_core_log_dbg(core_llr, LOG_NAME, "setup timeout work");

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "setup timeout work canceled");
		return;
	}

	if (sl_core_data_llr_state_get(core_llr) >= SL_CORE_LLR_STATE_SETUP) {
		sl_core_log_dbg(core_llr, LOG_NAME, "setup timeout work not needed");
		return;
	}

	if (core_llr->policy.options & SL_LLR_POLICY_OPT_INFINITE_START_TRIES) {
		sl_core_log_dbg(core_llr, LOG_NAME, "setup timeout work retrying");
		sl_core_timer_llr_begin(core_llr, SL_CORE_TIMER_LLR_SETUP);
		sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);
		sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);
		sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
		rtn = sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
		if (rtn)
			sl_core_log_err(core_llr, LOG_NAME,
				"setup timeout work llr_flgs_enable failed [%d]", rtn);
		return;
	}

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);

	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"setup timeout work llr_flgs_disable failed [%d]", rtn);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_SETUP_TIMEOUT);
	sl_core_hw_llr_setup_callback(core_llr);
}

void sl_core_hw_llr_setup_cancel_cmd(struct sl_core_llr *core_llr)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "setup cancel cmd");

	sl_core_llr_is_canceled_set(core_llr);

	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"setup cancel cmd llr_flgs_disable failed [%d]", rtn);

	rtn = sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_SETUP);
	if (rtn < 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"setup cancel cmd timer_llr_end failed [%d]", rtn);

	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_REUSE_TIMING]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_TIMEOUT]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_LOOP_TIME_INTR]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_UNEXP_LOOP_TIME_INTR]));

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT);

	sl_core_llr_is_canceled_clr(core_llr);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_CONFIGURED);
	sl_core_hw_llr_setup_callback(core_llr);
}

//---------------------------- START

void sl_core_hw_llr_start_cmd(struct sl_core_llr *core_llr,
	sl_core_llr_start_callback_t callback, void *tag, u32 flags)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "start cmd");

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);
	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);

	core_llr->tag             = tag;
	core_llr->callbacks.start = callback;
	core_llr->flags.start     = flags;
	sl_core_llr_is_canceled_clr(core_llr);

	sl_core_timer_llr_begin(core_llr, SL_CORE_TIMER_LLR_START);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"start cmd llr_flgs_disable failed [%d]", rtn);

	sl_core_work_llr_queue(core_llr, SL_CORE_WORK_LLR_START);
}

void sl_core_hw_llr_start_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_START]);

	sl_core_log_dbg(core_llr, LOG_NAME, "start work");

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start work canceled");
		return;
	}

	sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	rtn = sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0) {
		sl_core_log_err(core_llr, LOG_NAME,
			"start work llr_flgs_enable failed [%d]", rtn);
		rtn = sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_START);
		if (rtn < 0)
			sl_core_log_warn(core_llr, LOG_NAME,
				"start work timer_llr_end failed [%d]", rtn);
		sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
		sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_SETUP);
		sl_core_hw_llr_start_callback(core_llr);
		return;
	}

	sl_core_hw_llr_on(core_llr);
}

#define LLR_ADVANCE_TRIES 3
void sl_core_hw_llr_start_init_complete_intr_work(struct work_struct *work)
{
	int                 rtn;
	u32                 port;
	u64                 data64;
	struct sl_core_llr *core_llr;
	int                 x;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR]);

	port = core_llr->core_lgrp->num;

	sl_core_log_dbg(core_llr, LOG_NAME, "start init complete intr work (port = %d)", port);

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start init complete intr work canceled");
		return;
	}

	rtn = sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_START);
	if (rtn < 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"start init complete intr work timer_llr_end failed [%d]", rtn);

	sl_core_hw_llr_loop_time_stop(core_llr);

	for (x = 0; x < LLR_ADVANCE_TRIES; ++x) {
		sl_core_llr_read64(core_llr, SS2_PORT_PML_STS_LLR(core_llr->num), &data64);
		if (SS2_PORT_PML_STS_LLR_LLR_STATE_GET(data64) == SS2_PORT_PML_LLR_STATE_T_ADVANCE)
			break;
		usleep_range(15000, 20000);
	}

	msleep(20);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);

	if (x >= LLR_ADVANCE_TRIES) {
		sl_core_log_err(core_llr, LOG_NAME,
			"start init complete intr work not advancing (state = %lld)",
			SS2_PORT_PML_STS_LLR_LLR_STATE_GET(data64));
		sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_START_FAIL);
		sl_core_hw_llr_start_callback(core_llr);
		return;
	}

	sl_core_llr_read64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SM_REPLAY_CT_MAX_UPDATE(data64,
		core_llr->settings.replay_ct_max);
	data64 = SS2_PORT_PML_CFG_LLR_SM_REPLAY_TIMER_MAX_UPDATE(data64,
		core_llr->settings.replay_timer_max);
	sl_core_llr_write64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num), data64);
	sl_core_llr_flush64(core_llr, SS2_PORT_PML_CFG_LLR_SM(core_llr->num));

	sl_core_data_llr_info_map_set(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_RUNNING);
	sl_core_hw_llr_start_callback(core_llr);
}

void sl_core_hw_llr_start_timeout_work(struct work_struct *work)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = container_of(work, struct sl_core_llr, work[SL_CORE_WORK_LLR_START_TIMEOUT]);

	sl_core_log_dbg(core_llr, LOG_NAME, "start timeout work");

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start timeout work canceled");
		return;
	}

	if (sl_core_data_llr_state_get(core_llr) == SL_CORE_LLR_STATE_RUNNING) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start timeout work not needed");
		return;
	}

	if (core_llr->policy.options & SL_LLR_POLICY_OPT_INFINITE_START_TRIES) {
		sl_core_log_dbg(core_llr, LOG_NAME, "start timeout work retrying");
		sl_core_timer_llr_begin(core_llr, SL_CORE_TIMER_LLR_START);
		sl_core_hw_intr_llr_flgs_clr(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
		rtn = sl_core_hw_intr_llr_flgs_enable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
		if (rtn)
			sl_core_log_err(core_llr, LOG_NAME,
				"start timeout work llr_flgs_enable failed [%d]", rtn);
		return;
	}

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"start timeout work llr_flgs_disable failed [%d]", rtn);

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);
	sl_core_hw_llr_off(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_START_TIMEOUT);
	sl_core_hw_llr_start_callback(core_llr);
}

void sl_core_hw_llr_start_cancel_cmd(struct sl_core_llr *core_llr)
{
	int rtn;

	sl_core_log_dbg(core_llr, LOG_NAME, "start cancel cmd");

	sl_core_llr_is_canceled_set(core_llr);

	rtn = sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	if (rtn != 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"start cancel cmd llr_flgs_disable failed [%d]", rtn);

	rtn = sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_START);
	if (rtn < 0)
		sl_core_log_warn(core_llr, LOG_NAME,
			"start cancel cmd timer_llr_end failed [%d]", rtn);

	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START_TIMEOUT]));
	cancel_work_sync(&(core_llr->work[SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR]));

	sl_core_hw_llr_loop_time_stop(core_llr);
	sl_core_hw_llr_ordered_sets_stop(core_llr);
	sl_core_hw_llr_off(core_llr);
	sl_core_hw_llr_discard(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);

	sl_core_llr_is_canceled_clr(core_llr);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_CONFIGURED);
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

void sl_core_hw_llr_stop_cmd(struct sl_core_llr *core_llr, u32 flags)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "stop cmd");

	preempt_disable();

	sl_core_hw_llr_stop(core_llr);

	if (is_flag_set(flags, SL_CORE_LLR_FLAG_STOP_CLEAR_SETUP))
		sl_core_data_llr_data_clr(core_llr);

	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETTING_UP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_STARTING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_RUNNING);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_START_TIMEOUT);
	sl_core_data_llr_info_map_clr(core_llr, SL_CORE_INFO_MAP_LLR_REPLAY_MAX);

	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_CONFIGURED);

	preempt_enable();
}

void sl_core_hw_llr_off_wait(struct sl_core_llr *core_llr)
{
	int try_count;

	sl_core_log_dbg(core_llr, LOG_NAME, "off wait");

	try_count = 0;
	do {
		switch (sl_core_data_llr_state_get(core_llr)) {
		case SL_CORE_LLR_STATE_INVALID:
		case SL_CORE_LLR_STATE_OFF:
		case SL_CORE_LLR_STATE_CONFIGURED:
		case SL_CORE_LLR_STATE_START_FAIL:
			return;
		}
		usleep_range(1000, 2000);
	} while (try_count++ < 10 * 1000);
}
