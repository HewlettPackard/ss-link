// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/ktime.h>

#include "sl_kconfig.h"
#include "base/sl_core_work_link.h"
#include "base/sl_core_log.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "sl_ctl_link.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_link.h"
#include "hw/sl_core_hw_an.h"
#include "hw/sl_core_hw_an_up.h"
#include "hw/sl_core_hw_an_lp.h"
#include "hw/sl_core_hw_fec.h"
#include "hw/sl_core_hw_reset.h"
#include "hw/sl_core_hw_intr_flgs.h"
#include "hw/sl_core_hw_settings.h"
#include "hw/sl_core_hw_llr.h"

/* 0x01002510 (hni_pcs_corrected_cw)
 * 0x01002618 (hni_pcs_corrected_cw_bin_14)
 * (0x01002618 + 0x8) - 0x01002510 = 0x110 = Size
 */
#define SL_CORE_HW_FEC_CNTRS_OFFSET 0x01002510
#define SL_CORE_HW_FEC_CNTRS_SIZE   0x110

#define SL_CORE_HW_FEC_UP_SETTLE_WAIT_MS 250
#define SL_CORE_HW_FEC_UP_CHECK_WAIT_MS  500

static struct sl_core_link *core_links[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(core_links_lock);

#define LOG_NAME SL_CORE_DATA_LINK_LOG_NAME

#define SL_CORE_TIMER_INIT(_link, _timer_num, _work_num, _log)           \
	do {                                                             \
		(_link)->timers[_timer_num].data.link      = _link;      \
		(_link)->timers[_timer_num].data.timer_num = _timer_num; \
		(_link)->timers[_timer_num].data.work_num  = _work_num;  \
		strncpy((_link)->timers[_timer_num].data.log,            \
			_log, SL_CORE_TIMER_LINK_LOG_SIZE);              \
	} while (0)
#define SL_CORE_INTR_INIT(_link, _intr_num, _work_num, _log)           \
	do {                                                           \
		(_link)->intrs[_intr_num].flgs =                       \
			sl_core_hw_intr_flgs[_intr_num][(_link)->num]; \
		(_link)->intrs[_intr_num].data.link     = _link;       \
		(_link)->intrs[_intr_num].data.intr_num = _intr_num;   \
		(_link)->intrs[_intr_num].data.work_num = _work_num;   \
		strncpy((_link)->intrs[_intr_num].data.log,            \
			_log, SL_CORE_HW_INTR_LOG_SIZE);               \
	} while (0)

static int sl_core_data_link_init(struct sl_core_lgrp *core_lgrp, u8 link_num, struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME,
		"link init (link = 0x%p, lgrp = 0x%p)", core_link, core_lgrp);

	/* ----- general ----- */

	core_link->magic         = SL_CORE_LINK_MAGIC;
	core_link->num           = link_num;
	core_link->core_lgrp     = core_lgrp;
	core_link->an.fail_cause = SL_CORE_HW_AN_FAIL_CAUSE_NONE;
	spin_lock_init(&(core_link->data_lock));
	spin_lock_init(&(core_link->irq_data_lock));
	spin_lock_init(&(core_link->serdes.data_lock));

	/* ----- link ----- */

	spin_lock_init(&(core_link->link.data_lock));

	timer_setup(&(core_link->timers[SL_CORE_TIMER_LINK_UP].timer),
		sl_core_timer_link_timeout, 0);
	SL_CORE_TIMER_INIT(core_link, SL_CORE_TIMER_LINK_UP,
		SL_CORE_WORK_LINK_UP_TIMEOUT, "link up");
	timer_setup(&(core_link->timers[SL_CORE_TIMER_LINK_UP_CHECK].timer),
		sl_core_timer_link_timeout, 0);
	SL_CORE_TIMER_INIT(core_link, SL_CORE_TIMER_LINK_UP_CHECK,
		SL_CORE_WORK_LINK_UP_CHECK, "link up check");
	timer_setup(&(core_link->timers[SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER].timer),
		sl_core_timer_link_timeout, 0);
	SL_CORE_TIMER_INIT(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER,
		SL_CORE_WORK_LINK_UP, "link up xcvr high power");

	SL_CORE_INTR_INIT(core_link, SL_CORE_HW_INTR_LINK_UP,
		SL_CORE_WORK_LINK_UP_INTR, "link up");
	SL_CORE_INTR_INIT(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER,
		SL_CORE_WORK_LINK_HIGH_SER_INTR, "link high SER");
	SL_CORE_INTR_INIT(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION,
		SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR, "link llr max starvation");
	SL_CORE_INTR_INIT(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED,
		SL_CORE_WORK_LINK_LLR_STARVED_INTR, "link llr starved");
	SL_CORE_INTR_INIT(core_link, SL_CORE_HW_INTR_LINK_FAULT,
		SL_CORE_WORK_LINK_FAULT_INTR, "link fault");

	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_START]),
		sl_core_hw_an_up_start_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_START]),
		sl_core_hw_link_up_start_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP]),
		sl_core_hw_link_up_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_INTR]),
		sl_core_hw_link_up_intr_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_TIMEOUT]),
		sl_core_hw_link_up_timeout_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_CHECK]),
		sl_core_hw_link_up_check_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_HIGH_SER_INTR]),
		sl_core_hw_link_high_ser_intr_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR]),
		sl_core_hw_link_llr_max_starvation_intr_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_LLR_STARVED_INTR]),
		sl_core_hw_link_llr_starved_intr_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]),
		sl_core_hw_link_fault_intr_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_CANCEL]),
		sl_core_hw_link_up_cancel_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_FAIL]),
		sl_core_hw_link_up_fail_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_DOWN]),
		sl_core_hw_link_down_work);

	/* ----- an ----- */

	spin_lock_init(&(core_link->an.data_lock));
	core_link->an.lp_caps_state = SL_CORE_LINK_LP_CAPS_NOT_RUNNING;

	timer_setup(&(core_link->timers[SL_CORE_TIMER_LINK_AN_LP_CAPS_GET].timer),
		sl_core_timer_link_timeout, 0);
	SL_CORE_TIMER_INIT(core_link, SL_CORE_TIMER_LINK_AN_LP_CAPS_GET,
		SL_CORE_WORK_LINK_AN_LP_CAPS_GET_TIMEOUT, "an lp caps get");

	SL_CORE_INTR_INIT(core_link, SL_CORE_HW_INTR_AN_PAGE_RECV, 0, "an page recv");

	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET]),
		sl_core_hw_an_lp_caps_get_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET_TIMEOUT]),
		sl_core_hw_an_lp_caps_get_timeout_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET_DONE]),
		sl_core_hw_an_lp_caps_get_done_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_AN_UP]),
		sl_core_hw_an_up_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_DONE]),
		sl_core_hw_an_up_done_work);

	/* ----- fec ----- */

	timer_setup(&(core_link->timers[SL_CORE_TIMER_LINK_UP_FEC_SETTLE].timer),
		sl_core_timer_link_timeout, 0);
	SL_CORE_TIMER_INIT(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE,
		SL_CORE_WORK_LINK_UP_FEC_SETTLE, "link up fec settle");
	timer_setup(&(core_link->timers[SL_CORE_TIMER_LINK_UP_FEC_CHECK].timer),
		sl_core_timer_link_timeout, 0);
	SL_CORE_TIMER_INIT(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK,
		SL_CORE_WORK_LINK_UP_FEC_CHECK, "link up fec check");

	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]),
		sl_core_hw_link_up_fec_settle_work);
	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_CHECK]),
		sl_core_hw_link_up_fec_check_work);

	spin_lock_init(&core_link->fec.test_lock);

	atomic_set(&core_link->an.retry_count, 0);

	/* ----- ald ----- */

	SL_CORE_INTR_INIT(core_link, SL_CORE_HW_INTR_LANE_DEGRADE,
		SL_CORE_WORK_LINK_LANE_DEGRADE_INTR, "lane degrade");

	INIT_WORK(&(core_link->work[SL_CORE_WORK_LINK_LANE_DEGRADE_INTR]),
		sl_core_hw_link_lane_degrade_intr_work);

	/* do very last thing */
	core_link->link.state = SL_CORE_LINK_STATE_UNCONFIGURED;
	sl_media_jack_link_led_set(core_link->core_lgrp->core_ldev->num,
		core_link->core_lgrp->num, SL_CORE_LINK_STATE_UNCONFIGURED);

	return 0;
}

int sl_core_data_link_new(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                  rtn;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	struct sl_core_ldev *core_ldev;
	u8                   is_configuring;

	core_ldev = sl_core_ldev_get(ldev_num);
	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
	spin_lock(&core_lgrp->data_lock);
	is_configuring = core_lgrp->is_configuring;
	spin_unlock(&core_lgrp->data_lock);
	if (is_configuring) {
		sl_core_log_err(core_lgrp, LOG_NAME, "lgrp is configuring");
		return -EBADRQC;
	}

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	if (core_link) {
		sl_core_log_err(core_link, LOG_NAME, "exists (link = 0x%p)", core_link);
		return -EBADRQC;
	}

	core_link = kzalloc(sizeof(struct sl_core_link), GFP_KERNEL);
	if (core_link == NULL)
		return -ENOMEM;

	rtn = sl_core_data_link_init(core_lgrp, link_num, core_link);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME, "core_data_link_init failed [%d]", rtn);
		kfree(core_link);
		return rtn;
	}

	rtn = sl_core_hw_intr_hdlr_register(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME, "core_hw_intr_hdlr_register failed [%d]", rtn);
		kfree(core_link);
		return rtn;
	}

	sl_core_hw_llr_link_init(core_link);

	if (core_ldev->ops.dmac_alloc) {
		rtn = core_ldev->ops.dmac_alloc(
			core_ldev->accessors.dmac,
			SL_CORE_HW_FEC_CNTRS_OFFSET,
			SL_CORE_HW_FEC_CNTRS_SIZE);
		if (rtn) {
			sl_core_log_err(core_link, LOG_NAME, "dmac_alloc failed [%d]", rtn);
			kfree(core_link);
			return rtn;
		}
	}

	sl_core_log_dbg(core_link, LOG_NAME, "new (link = 0x%p)", core_link);

	spin_lock(&core_links_lock);
	core_links[ldev_num][lgrp_num][link_num] = core_link;
	spin_unlock(&core_links_lock);

	return 0;
}

static void sl_core_data_link_free(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME,
		"link free (link = 0x%p, lgrp = 0x%p)", core_link, core_link->core_lgrp);

	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_AN_LP_CAPS_GET);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER);

	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET_TIMEOUT]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET_DONE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_DONE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_TIMEOUT]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FAIL]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CANCEL]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_DOWN]));

	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);

	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_HIGH_SER_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_STARVED_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]));
}

void sl_core_data_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	struct sl_core_ldev *core_ldev;
	u8                   is_configuring;

	core_ldev = sl_core_ldev_get(ldev_num);
	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
	spin_lock(&core_lgrp->data_lock);
	is_configuring = core_lgrp->is_configuring;
	spin_unlock(&core_lgrp->data_lock);
	if (is_configuring) {
		sl_core_log_err(core_lgrp, LOG_NAME, "lgrp is configuring");
		return;
	}

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
	if (!core_link) {
		sl_core_log_dbg(NULL, LOG_NAME,
			"not found (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return;
	}

	spin_lock(&core_links_lock);
	core_links[ldev_num][lgrp_num][link_num] = NULL;
	spin_unlock(&core_links_lock);

	sl_core_log_dbg(core_link, LOG_NAME, "del (link = 0x%p)", core_link);

	sl_core_data_link_free(core_link);

	sl_core_hw_intr_hdlr_unregister(core_link);

	if (core_ldev->ops.dmac_free)
		core_ldev->ops.dmac_free(core_ldev->accessors.dmac);

	kfree(core_link);
}

struct sl_core_link *sl_core_data_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_core_link *core_link;

	spin_lock(&core_links_lock);
	core_link = core_links[ldev_num][lgrp_num][link_num];
	spin_unlock(&core_links_lock);

	sl_core_log_dbg(core_link, LOG_NAME, "get (link = 0x%p)", core_link);

	return core_link;
}

void sl_core_data_link_config_set(struct sl_core_link *core_link,
	struct sl_core_link_config *link_config)
{
	sl_core_log_dbg(core_link, LOG_NAME,
		"config set (flags = 0x%X, hpe_map = 0x%X, pause_map = 0x%X)",
		link_config->flags, link_config->hpe_map, link_config->pause_map);

	spin_lock(&core_link->data_lock);
	core_link->config = *link_config;
	spin_unlock(&core_link->data_lock);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_CONFIGURED);
}

int sl_core_data_link_settings(struct sl_core_link *core_link)
{
	struct sl_lgrp_config *lgrp_config;
	struct sl_link_caps   *link_caps;

	sl_core_log_dbg(core_link, LOG_NAME, "settings");

	lgrp_config = &(core_link->core_lgrp->config);
	link_caps   = &(core_link->core_lgrp->link_caps[core_link->num]);

	sl_core_log_dbg(core_link, LOG_NAME,
		"settings - tech map  = 0x%08X", link_caps->tech_map);
	sl_core_log_dbg(core_link, LOG_NAME,
		"settings - fec map   = 0x%08X", link_caps->fec_map);
	sl_core_log_dbg(core_link, LOG_NAME,
		"settings - pause map = 0x%08X", link_caps->pause_map);
	sl_core_log_dbg(core_link, LOG_NAME,
		"settings - hpe map   = 0x%08X", link_caps->hpe_map);
	sl_core_log_dbg(core_link, LOG_NAME,
		"settings - furcation = %u", lgrp_config->furcation);
	sl_core_log_dbg(core_link, LOG_NAME,
		"settings - options   = %u", lgrp_config->options);

	/* abilities */
	if (hweight_long(link_caps->tech_map) != 1) {
		sl_core_log_err(core_link, LOG_NAME,
			"settings - tech map invalid (map = 0x%08X)",
			link_caps->tech_map);
		return -EINVAL;
	}
	if (hweight_long(link_caps->fec_map) > 1) {
		sl_core_log_err(core_link, LOG_NAME,
			"settings - fec map invalid (map = 0x%08X)",
			link_caps->fec_map);
		return -EINVAL;
	}

	/* PCS */
#ifdef BUILDSYS_FRAMEWORK_CASSINI
	core_link->pcs.settings.clock_period = 0x38D;
#else /* rosetta */
	core_link->pcs.settings.clock_period = 0x3E8;
#endif /* BUILDSYS_FRAMEWORK_CASSINI */
	if (SL_LGRP_CONFIG_TECH_CK_400G & link_caps->tech_map) {
		core_link->pcs.settings.speed                      = SL_LGRP_CONFIG_TECH_CK_400G;
		core_link->pcs.settings.pcs_mode                   = SL_CORE_HW_PCS_MODE_CK_400G;
		core_link->pcs.settings.rx_active_lanes            = SL_CORE_HW_ACTIVE_LANES_CK_400G;
		if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_FABRIC)) {
			core_link->pcs.settings.tx_cdc_ready_level         = 8;
			core_link->pcs.settings.tx_en_pk_bw_limiter        = 1;
			core_link->pcs.settings.tx_gearbox_credits         = 12;
			core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
			core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
			core_link->pcs.settings.cw_gap                     = 6;
		} else {
			core_link->pcs.settings.tx_cdc_ready_level         = 8;
			core_link->pcs.settings.tx_en_pk_bw_limiter        = 1;
			core_link->pcs.settings.tx_gearbox_credits         = 8;
			core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
			core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
			core_link->pcs.settings.cw_gap                     = 6;
		}
		if (sl_core_link_config_is_enable_ald_set(core_link))
			core_link->pcs.settings.tx_gearbox_credits = 12;
	}
	if (SL_LGRP_CONFIG_TECH_CK_200G & link_caps->tech_map) {
		core_link->pcs.settings.speed                      = SL_LGRP_CONFIG_TECH_CK_200G;
		core_link->pcs.settings.pcs_mode                   = SL_CORE_HW_PCS_MODE_CK_200G;
		core_link->pcs.settings.rx_active_lanes            = SL_CORE_HW_ACTIVE_LANES_CK_200G(core_link->num);
		core_link->pcs.settings.tx_cdc_ready_level         = 8;
		core_link->pcs.settings.tx_en_pk_bw_limiter        = 1;
		core_link->pcs.settings.tx_gearbox_credits         = 8;
		core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
		core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
		core_link->pcs.settings.cw_gap                     = 6;
	}
	if (SL_LGRP_CONFIG_TECH_CK_100G & link_caps->tech_map) {
		core_link->pcs.settings.speed                      = SL_LGRP_CONFIG_TECH_CK_100G;
		core_link->pcs.settings.pcs_mode                   = SL_CORE_HW_PCS_MODE_CK_100G;
		core_link->pcs.settings.rx_active_lanes            = SL_CORE_HW_ACTIVE_LANES_CK_100G(core_link->num);
		core_link->pcs.settings.tx_cdc_ready_level         = 8;
		core_link->pcs.settings.tx_en_pk_bw_limiter        = 1;
		core_link->pcs.settings.tx_gearbox_credits         = 4;
		core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
		core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
		core_link->pcs.settings.cw_gap                     = 16;
	}
	if (SL_LGRP_CONFIG_TECH_BS_200G & link_caps->tech_map) {
		core_link->pcs.settings.speed                      = SL_LGRP_CONFIG_TECH_BS_200G;
		core_link->pcs.settings.pcs_mode                   = SL_CORE_HW_PCS_MODE_BS_200G;
		core_link->pcs.settings.rx_active_lanes            = SL_CORE_HW_ACTIVE_LANES_BS_200G;
		if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_FABRIC)) {
			core_link->pcs.settings.tx_cdc_ready_level         = 8;
			core_link->pcs.settings.tx_en_pk_bw_limiter        = 0;
			core_link->pcs.settings.tx_gearbox_credits         = 12;
			core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
			core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
			core_link->pcs.settings.cw_gap                     = 6;
		} else {
			core_link->pcs.settings.tx_cdc_ready_level         = 8;
			core_link->pcs.settings.tx_en_pk_bw_limiter        = 0;
			core_link->pcs.settings.tx_gearbox_credits         = 8;
			core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
			core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
			if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_R1))
				core_link->pcs.settings.cw_gap             = 24;
			else
				core_link->pcs.settings.cw_gap             = 6;
		}
		if (sl_core_link_config_is_enable_ald_set(core_link))
			core_link->pcs.settings.tx_gearbox_credits = 12;
	}
	if (SL_LGRP_CONFIG_TECH_CD_100G & link_caps->tech_map) {
		core_link->pcs.settings.speed                      = SL_LGRP_CONFIG_TECH_CD_100G;
		core_link->pcs.settings.pcs_mode                   = SL_CORE_HW_PCS_MODE_CD_100G;
		core_link->pcs.settings.rx_active_lanes            = SL_CORE_HW_ACTIVE_LANES_CD_100G(core_link->num);
		core_link->pcs.settings.tx_cdc_ready_level         = 8;
		core_link->pcs.settings.tx_en_pk_bw_limiter        = 1;
		core_link->pcs.settings.tx_gearbox_credits         = 4;
		core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
		core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
		core_link->pcs.settings.cw_gap                     = 6;
	}
	if (SL_LGRP_CONFIG_TECH_CD_50G & link_caps->tech_map) {
		core_link->pcs.settings.speed                      = SL_LGRP_CONFIG_TECH_CD_50G;
		core_link->pcs.settings.pcs_mode                   = SL_CORE_HW_PCS_MODE_CD_50G;
		core_link->pcs.settings.rx_active_lanes            = SL_CORE_HW_ACTIVE_LANES_CD_50G(core_link->num);
		core_link->pcs.settings.tx_cdc_ready_level         = 8;
		core_link->pcs.settings.tx_en_pk_bw_limiter        = 1;
		core_link->pcs.settings.tx_gearbox_credits         = 2;
		core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
		core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
		core_link->pcs.settings.cw_gap                     = 12;
	}
	if (SL_LGRP_CONFIG_TECH_BJ_100G & link_caps->tech_map) {
		core_link->pcs.settings.speed                      = SL_LGRP_CONFIG_TECH_BJ_100G;
		core_link->pcs.settings.pcs_mode                   = SL_CORE_HW_PCS_MODE_BJ_100G;
		core_link->pcs.settings.rx_active_lanes            = SL_CORE_HW_ACTIVE_LANES_BJ_100G;
		core_link->pcs.settings.tx_cdc_ready_level         = 8;
		core_link->pcs.settings.tx_en_pk_bw_limiter        = 0;
		core_link->pcs.settings.tx_gearbox_credits         = 8;
		core_link->pcs.settings.rx_restart_lock_on_bad_cws = 0;
		core_link->pcs.settings.rx_restart_lock_on_bad_ams = 1;
		core_link->pcs.settings.cw_gap                     = 6;
	}

	if (is_flag_set(lgrp_config->options, SL_LGRP_CONFIG_OPT_FABRIC)) {
		core_link->pcs.settings.mac_tx_credits = 8;
	} else {
		switch (lgrp_config->furcation) {
		case SL_MEDIA_FURCATION_X1:
			core_link->pcs.settings.mac_tx_credits = 8;
			break;
		case SL_MEDIA_FURCATION_X2:
			core_link->pcs.settings.mac_tx_credits = 4;
			break;
		case SL_MEDIA_FURCATION_X4:
			core_link->pcs.settings.mac_tx_credits = 2;
			break;
		}
	}

	switch (lgrp_config->furcation) {
	case SL_MEDIA_FURCATION_X1:
		core_link->pcs.settings.underrun_clr = 0;
		break;
	case SL_MEDIA_FURCATION_X2:
		core_link->pcs.settings.underrun_clr = (~(3 << (core_link->num * 2)) & 0xF);
		break;
	case SL_MEDIA_FURCATION_X4:
		core_link->pcs.settings.underrun_clr = (~(1 << core_link->num) & 0xF);
		break;
	}

	/* FEC */
	if (SL_LGRP_CONFIG_FEC_RS_LL & link_caps->fec_map) {
		core_link->pcs.settings.rs_mode  = SL_CORE_HW_RS_MODE_CORRECT;
		core_link->fec.settings.mode     = SL_LGRP_FEC_MODE_CORRECT;
		core_link->fec.settings.type     = SL_LGRP_CONFIG_FEC_RS_LL;
	} else if (SL_LGRP_CONFIG_FEC_RS & link_caps->fec_map) {
		core_link->pcs.settings.rs_mode  = SL_CORE_HW_RS_MODE_CORRECT;
		core_link->fec.settings.mode     = SL_LGRP_FEC_MODE_CORRECT;
		core_link->fec.settings.type     = SL_LGRP_CONFIG_FEC_RS;
	} else {
		core_link->pcs.settings.rs_mode  = SL_CORE_HW_RS_MODE_OFF;
		core_link->fec.settings.mode     = SL_LGRP_FEC_MODE_OFF;
		core_link->fec.settings.type     = 0;
	}

	core_link->fec.settings.up_settle_wait_ms = (core_link->config.fec_up_settle_wait_ms < 0) ?
		SL_CORE_HW_FEC_UP_SETTLE_WAIT_MS : core_link->config.fec_up_settle_wait_ms;
	core_link->fec.settings.up_check_wait_ms  = (core_link->config.fec_up_check_wait_ms < 0) ?
		SL_CORE_HW_FEC_UP_CHECK_WAIT_MS : core_link->config.fec_up_check_wait_ms;

	if (core_link->config.fec_up_ucw_limit < 0)
		core_link->fec.settings.up_ucw_limit =
			sl_ctl_link_fec_limit_calc(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num), SL_CTL_LINK_FEC_UCW_MANT, SL_CTL_LINK_FEC_UCW_EXP);
	else
		core_link->fec.settings.up_ucw_limit = core_link->config.fec_up_ucw_limit;

	if (core_link->config.fec_up_ccw_limit < 0)
		core_link->fec.settings.up_ccw_limit = 0;
	else
		core_link->fec.settings.up_ccw_limit = core_link->config.fec_up_ccw_limit;

	return 0;
}

void sl_core_data_link_timeouts(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "timeouts");

	if (core_link->config.link_up_timeout_ms == 0) {
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"timeouts - link up timeout is 0, setting to default");
		core_link->timers[SL_CORE_TIMER_LINK_UP].data.timeout_ms = 10000;
	} else {
		core_link->timers[SL_CORE_TIMER_LINK_UP].data.timeout_ms = core_link->config.link_up_timeout_ms;
	}

	core_link->timers[SL_CORE_TIMER_LINK_UP_CHECK].data.timeout_ms = 100;

	core_link->timers[SL_CORE_TIMER_LINK_UP_FEC_SETTLE].data.timeout_ms = core_link->fec.settings.up_settle_wait_ms;
	core_link->timers[SL_CORE_TIMER_LINK_UP_FEC_CHECK].data.timeout_ms  = core_link->fec.settings.up_check_wait_ms;
}

void sl_core_data_link_state_set(struct sl_core_link *core_link, u32 link_state)
{
	spin_lock(&core_link->link.data_lock);
	core_link->link.state = link_state;
	spin_unlock(&core_link->link.data_lock);

	sl_media_jack_link_led_set(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num, link_state);

	sl_core_log_dbg(core_link, LOG_NAME,
		"set state = %s", sl_core_link_state_str(link_state));
}

u32 sl_core_data_link_state_get(struct sl_core_link *core_link)
{
	u32 link_state;

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"get state = %u", link_state);

	return link_state;
}

u32 sl_core_data_link_speed_get(struct sl_core_link *core_link)
{
	u32 link_speed;

	spin_lock(&core_link->link.data_lock);
	link_speed = core_link->pcs.settings.speed;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"get speed = %u", link_speed);

	return link_speed;
}

u16 sl_core_data_link_clocking_get(struct sl_core_link *core_link)
{
	u16 link_clocking;

	spin_lock(&core_link->link.data_lock);
	link_clocking = core_link->serdes.core_serdes_settings.clocking;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"get clocking = %u", link_clocking);

	return link_clocking;
}

void sl_core_data_link_info_map_clr(struct sl_core_link *core_link, u32 bit_num)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_link->irq_data_lock, irq_flags);

	if (bit_num == SL_CORE_INFO_MAP_NUM_BITS)
		bitmap_zero((unsigned long *)&(core_link->info_map), SL_CORE_INFO_MAP_NUM_BITS);
	else
		clear_bit(bit_num, (unsigned long *)&(core_link->info_map));

	sl_core_log_dbg(core_link, LOG_NAME,
		"clr info map 0x%016llX", core_link->info_map);

	spin_unlock_irqrestore(&core_link->irq_data_lock, irq_flags);
}

void sl_core_data_link_info_map_set(struct sl_core_link *core_link, u32 bit_num)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_link->irq_data_lock, irq_flags);

	set_bit(bit_num, (unsigned long *)&(core_link->info_map));

	sl_core_log_dbg(core_link, LOG_NAME,
		"set info map 0x%016llX", core_link->info_map);

	spin_unlock_irqrestore(&core_link->irq_data_lock, irq_flags);
}

u64 sl_core_data_link_info_map_get(struct sl_core_link *core_link)
{
	u64           info_map;
	unsigned long irq_flags;

	spin_lock_irqsave(&core_link->irq_data_lock, irq_flags);
	info_map = core_link->info_map;
	spin_unlock_irqrestore(&core_link->irq_data_lock, irq_flags);

	sl_core_log_dbg(core_link, LOG_NAME,
		"get info map 0x%016llX", info_map);

	return info_map;
}

void sl_core_data_link_last_up_fail_cause_map_set(struct sl_core_link *core_link, u64 up_fail_cause_map)
{
	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME,
			"last_up_fail_cause_map_set ignoring (up_fail_cause_map = 0x%llX)", up_fail_cause_map);
		return;
	}

	spin_lock(&core_link->link.data_lock);
	core_link->link.last_up_fail_cause_map |= up_fail_cause_map;
	core_link->link.last_up_fail_time       = ktime_get_real_seconds();
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"last up fail cause set (cause_map = 0x%llX)", up_fail_cause_map);
}

void sl_core_data_link_last_up_fail_info_get(struct sl_core_link *core_link, u64 *up_fail_cause_map,
	time64_t *up_fail_time)
{
	spin_lock(&core_link->link.data_lock);
	*up_fail_cause_map = core_link->link.last_up_fail_cause_map;
	*up_fail_time      = core_link->link.last_up_fail_time;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"last up fail cause get (cause_map = 0x%llX)", *up_fail_cause_map);
}

u64 sl_core_data_link_last_up_fail_cause_map_get(struct sl_core_link *core_link)
{
	u64 up_fail_cause_map;

	spin_lock(&core_link->link.data_lock);
	up_fail_cause_map = core_link->link.last_up_fail_cause_map;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"last up fail cause get (up_fail_cause_map = 0x%llX)", up_fail_cause_map);

	return up_fail_cause_map;
}

void sl_core_data_link_is_last_down_new_set(struct sl_core_link *core_link, bool is_last_down_new)
{
	spin_lock(&core_link->link.data_lock);
	core_link->link.is_last_down_new = is_last_down_new;
	spin_unlock(&core_link->link.data_lock);
}

void sl_core_data_link_last_down_cause_map_set(struct sl_core_link *core_link, u64 down_cause_map)
{
	spin_lock(&core_link->link.data_lock);

	if (core_link->link.is_last_down_new) {
		core_link->link.last_down_cause_map = 0;
		core_link->link.is_last_down_new = false;
	}

	core_link->link.last_down_cause_map |= down_cause_map;
	core_link->link.last_down_time       = ktime_get_real_seconds();
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"last down cause map set (down_cause_map = 0x%llX)", down_cause_map);
}

void sl_core_data_link_last_down_cause_map_info_get(struct sl_core_link *core_link, u64 *down_cause_map,
						    time64_t *down_time)
{
	spin_lock(&core_link->link.data_lock);
	*down_cause_map = core_link->link.last_down_cause_map;
	*down_time      = core_link->link.last_down_time;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"last down cause map info get (down_cause_map = 0x%llX, down_time = %lld %ptTt %ptTd)",
		*down_cause_map, *down_time, down_time, down_time);
}

u64 sl_core_data_link_last_down_cause_map_get(struct sl_core_link *core_link)
{
	u64 down_cause_map;

	spin_lock(&core_link->link.data_lock);
	down_cause_map = core_link->link.last_down_cause_map;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"last down cause map get (down_cause_map = 0x%llX)", down_cause_map);

	return down_cause_map;
}

void sl_core_data_link_ccw_warn_limit_crossed_set(struct sl_core_link *core_link, bool is_limit_crossed)
{
	spin_lock(&core_link->link.data_lock);
	core_link->link.is_ccw_warn_limit_crossed = is_limit_crossed;
	core_link->link.last_ccw_warn_limit_crossed_time  = ktime_get_real_seconds();
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"ccw warn limit crossed set (is_limit_crossed = %d %s)", core_link->link.is_ccw_warn_limit_crossed,
		core_link->link.is_ccw_warn_limit_crossed ? "yes":"no");
}

void sl_core_data_link_ccw_warn_limit_crossed_get(struct sl_core_link *core_link, bool *is_limit_crossed,
	time64_t *limit_crossed_time)
{
	spin_lock(&core_link->link.data_lock);
	*is_limit_crossed = core_link->link.is_ccw_warn_limit_crossed;
	*limit_crossed_time = core_link->link.last_ccw_warn_limit_crossed_time;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"ccw warn limit crossed get (is_limit_crossed = %d %s)", *is_limit_crossed,
		*is_limit_crossed ? "yes":"no");
}

void sl_core_data_link_ucw_warn_limit_crossed_set(struct sl_core_link *core_link, bool is_limit_crossed)
{
	spin_lock(&core_link->link.data_lock);
	core_link->link.is_ucw_warn_limit_crossed = is_limit_crossed;
	core_link->link.last_ucw_warn_limit_crossed_time  = ktime_get_real_seconds();
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"ucw warn limit crossed set (is_limit_crossed = %d %s)", core_link->link.is_ucw_warn_limit_crossed,
		core_link->link.is_ucw_warn_limit_crossed ? "yes":"no");
}

void sl_core_data_link_ucw_warn_limit_crossed_get(struct sl_core_link *core_link, bool *is_limit_crossed,
	time64_t *limit_crossed_time)
{
	spin_lock(&core_link->link.data_lock);
	*is_limit_crossed = core_link->link.is_ucw_warn_limit_crossed;
	*limit_crossed_time = core_link->link.last_ucw_warn_limit_crossed_time;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"ucw warn limit crossed get (is_limit_crossed = %d %s)", *is_limit_crossed,
		*is_limit_crossed ? "yes":"no");
}

u32 sl_core_data_link_fec_mode_get(struct sl_core_link *core_link)
{
	u32 fec_mode;

	spin_lock(&core_link->link.data_lock);
	fec_mode = core_link->fec.settings.mode;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"fec_mode get = %u %s", fec_mode, sl_lgrp_fec_mode_str(fec_mode));

	return fec_mode;
}

u32 sl_core_data_link_fec_type_get(struct sl_core_link *core_link)
{
	u32 fec_type;

	spin_lock(&core_link->link.data_lock);
	fec_type = core_link->fec.settings.type;
	spin_unlock(&core_link->link.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME,
		"fec_type get = %u %s", fec_type, sl_lgrp_config_fec_str(fec_type));

	return fec_type;
}

u32 sl_core_data_link_an_lp_caps_state_get(struct sl_core_link *core_link)
{
	u32 lp_caps_state;

	spin_lock(&core_link->an.data_lock);
	lp_caps_state = core_link->an.lp_caps_state;
	spin_unlock(&core_link->an.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps state get (lp_caps_state = %u %s)",
		lp_caps_state, sl_link_an_lp_caps_state_str(lp_caps_state));

	return lp_caps_state;
}

void sl_core_data_link_an_lp_caps_state_set(struct sl_core_link *core_link, u32 lp_caps_state)
{
	spin_lock(&core_link->an.data_lock);
	core_link->an.lp_caps_state = lp_caps_state;
	spin_unlock(&core_link->an.data_lock);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps state set (lp_caps_state = %u %s)",
		lp_caps_state, sl_link_an_lp_caps_state_str(lp_caps_state));
}

void sl_core_data_link_an_fail_cause_get(struct sl_core_link *core_link, u32 *fail_cause, time64_t *fail_time)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_link->irq_data_lock, irq_flags);
	*fail_cause = core_link->an.fail_cause;
	*fail_time  = core_link->an.fail_time;
	spin_unlock_irqrestore(&core_link->irq_data_lock, irq_flags);

	sl_core_log_dbg(core_link, LOG_NAME, "an fail cause get (fail_cause = %u %s)",
		*fail_cause, sl_core_link_an_fail_cause_str(*fail_cause));
}

void sl_core_data_link_an_fail_cause_set(struct sl_core_link *core_link, u32 fail_cause)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_link->irq_data_lock, irq_flags);
	core_link->an.fail_cause = fail_cause;
	core_link->an.fail_time  = ktime_get_real_seconds();
	spin_unlock_irqrestore(&core_link->irq_data_lock, irq_flags);

	sl_core_log_dbg(core_link, LOG_NAME, "an fail cause set (fail_cause = %u %s)",
		fail_cause, sl_core_link_an_fail_cause_str(fail_cause));
}

u32 sl_core_data_link_an_retry_count_get(struct sl_core_link *core_link)
{
	u32 retry_count;

	retry_count = atomic_read(&core_link->an.retry_count);

	sl_core_log_dbg(core_link, LOG_NAME, "an retry count get (retry_count = %u)", retry_count);

	return retry_count;
}
