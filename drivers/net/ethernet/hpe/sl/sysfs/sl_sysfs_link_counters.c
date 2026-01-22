// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_core_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_link_counters.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t link_up_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP_CMD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_retry_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP_RETRY, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up retry show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_DOWN_CMD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link down cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_DOWN, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link down show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_canceled_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP_CANCELED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up canceled show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_cancel_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP_CANCEL_CMD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up cancel cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_reset_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_RESET_CMD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link reset cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_fault_async_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_FAULT_ASYNC, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link fault async show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_ccw_warn_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_CCW_WARN_CROSSED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link ccw warn crossed show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_ucw_warn_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UCW_WARN_CROSSED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link ucw warn crossed show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_ccw_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_DOWN_CCW_LIMIT_CROSSED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link down ccw limit crossed show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_ucw_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_DOWN_UCW_LIMIT_CROSSED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link down ucw limit crossed show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_ccw_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_DOWN_CCW_CAUSE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link down ccw cause show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_down_ucw_cause_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_DOWN_UCW_CAUSE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link down ucw cause show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_fail_ccw_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP_FAIL_CCW_LIMIT_CROSSED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up fail ccw limit crossed show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_up_fail_ucw_limit_crossed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_UP_FAIL_UCW_LIMIT_CROSSED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link up fail ucw limit crossed show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_autoneg_np_retry_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_retry_count_get(ctrl_link, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link autoneg np retry show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t link_autoneg_attempt_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_counters_get(ctrl_link, LINK_HW_AN_ATTEMPT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link autoneg attempt show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_ucw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_UCW, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause ucw show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_lf_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_LF, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause lf show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_rf_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_RF, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause rf show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_down_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_DOWN, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause down show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_up_tries_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_UP_TRIES, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause up tries show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_autoneg_nomatch_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_AUTONEG_NOMATCH, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause autoneg nomatch show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_autoneg_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_AUTONEG, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause autoneg show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_config_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_CONFIG, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause config show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_intr_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_INTR_ENABLE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause intr enable show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_timeout_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_TIMEOUT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause timeout show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_canceled_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_CANCELED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause canceled show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_unsupported_cable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_UNSUPPORTED_CABLE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause unsupported cable show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_command_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_COMMAND, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause command show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_downshift_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_DOWNSHIFT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause downshift show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_llr_replay_max_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_LLR_REPLAY_MAX, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause llr replay max show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_upshift_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_UPSHIFT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause upshift show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_autoneg_config_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_AUTONEG_CONFIG, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause autoneg config show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_pcs_fault_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_PCS_FAULT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause pcs fault show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_serdes_pll_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_SERDES_PLL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause serdes pll show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_serdes_config_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_SERDES_CONFIG, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause serdes config show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_serdes_signal_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_SERDES_SIGNAL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause serdes signal show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_serdes_quality_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_SERDES_QUALITY, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause serdes quality show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_no_media_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_NO_MEDIA, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause no media show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_ccw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_CCW, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause ccw show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_high_temp_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_HIGH_TEMP, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause high temp show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_intr_register_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_INTR_REGISTER, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause intr register show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_media_error_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_MEDIA_ERROR, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause media error show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_up_canceled_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_UP_CANCELED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause up canceled show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_unsupported_speed_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_UNSUPPORTED_SPEED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause unsupported speed show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_ss200_cable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_SS200_CABLE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause ss200 cable show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_tx_lol_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_TX_LOL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause tx lol show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_rx_lol_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_RX_LOL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause rx lol show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_tx_los_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_TX_LOS, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause tx los show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_rx_los_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_cause_counters_get(ctrl_link, LINK_CAUSE_RX_LOS, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link cause rx los show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_lp_caps_serdes_link_up_fail_show(struct kobject *kobj,
							 struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_LP_CAPS_SERDES_LINK_UP_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause lp caps serdes link up fail show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_lp_caps_not_complete_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_LP_CAPS_NOT_COMPLETE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause lp caps not complete show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_not_complete_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_NOT_COMPLETE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause not complete show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_test_caps_nomatch_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_TEST_CAPS_NOMATCH, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause test caps nomatch show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_serdes_link_up_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_SERDES_LINK_UP_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause serdes link up fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_bp_store_state_bad_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_BP_STORE_STATE_BAD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause bp store state bad show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_bp_store_lp_ability_not_set_show(struct kobject *kobj, struct kobj_attribute *kattr,
							 char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_BP_STORE_LP_ABILITY_NOT_SET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause bp store lp ability not set show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_bp_store_state_error_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_BP_STORE_STATE_ERROR, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause bp store state error show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_bp_store_bp_not_set_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_BP_STORE_BP_NOT_SET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause bp store bp not set show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_bp_send_intr_enable_fail_show(struct kobject *kobj, struct kobj_attribute *kattr,
						      char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_BP_SEND_INTR_ENABLE_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause bp send intr enable fail show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_np_store_state_bad_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_NP_STORE_STATE_BAD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause np store state bad show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_np_store_bp_set_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_NP_STORE_BP_SET, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause np store state bp set show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_np_check_state_bad_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_NP_CHECK_STATE_BAD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause np check state bad show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_intr_state_invalid_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_INTR_STATE_INVALID, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause intr state invalid show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_intr_an_retry_np_send_fail_show(struct kobject *kobj, struct kobj_attribute *kattr,
							char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_INTR_AN_RETRY_NP_SEND_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause intr an retry np send fail show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_intr_out_of_pages_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_INTR_OUT_OF_PAGES, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause intr out og pages show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_intr_np_send_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_INTR_NP_SEND_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause intr np send fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_pages_decode_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause pages decode fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_pages_decode_no_bp_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_NO_BP, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause pages decode no bp show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t an_cause_pages_decode_oui_invalid_show(struct kobject *kobj, struct kobj_attribute *kattr,
						      char *buf)
{
	struct sl_ctrl_link *ctrl_link;
	u32                  counter;
	int                  rtn;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, counters_kobj);

	rtn = sl_ctrl_link_an_cause_counters_get(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_OUI_INVALID, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link an cause pages decode oui invalid show (counter = %u)",
		   counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static struct kobj_attribute link_up_cmd                    = __ATTR_RO(link_up_cmd);
static struct kobj_attribute link_up_retry                  = __ATTR_RO(link_up_retry);
static struct kobj_attribute link_up                        = __ATTR_RO(link_up);
static struct kobj_attribute link_up_fail                   = __ATTR_RO(link_up_fail);
static struct kobj_attribute link_down_cmd                  = __ATTR_RO(link_down_cmd);
static struct kobj_attribute link_down                      = __ATTR_RO(link_down);
static struct kobj_attribute link_up_cancel_cmd             = __ATTR_RO(link_up_cancel_cmd);
static struct kobj_attribute link_up_canceled               = __ATTR_RO(link_up_canceled);
static struct kobj_attribute link_reset_cmd                 = __ATTR_RO(link_reset_cmd);
static struct kobj_attribute link_fault_async               = __ATTR_RO(link_fault_async);
static struct kobj_attribute link_ccw_warn_crossed          = __ATTR_RO(link_ccw_warn_crossed);
static struct kobj_attribute link_ucw_warn_crossed          = __ATTR_RO(link_ucw_warn_crossed);
static struct kobj_attribute link_down_ccw_limit_crossed    = __ATTR_RO(link_down_ccw_limit_crossed);
static struct kobj_attribute link_down_ucw_limit_crossed    = __ATTR_RO(link_down_ucw_limit_crossed);
static struct kobj_attribute link_down_ccw_cause            = __ATTR_RO(link_down_ccw_cause);
static struct kobj_attribute link_down_ucw_cause            = __ATTR_RO(link_down_ucw_cause);
static struct kobj_attribute link_up_fail_ccw_limit_crossed = __ATTR_RO(link_up_fail_ccw_limit_crossed);
static struct kobj_attribute link_up_fail_ucw_limit_crossed = __ATTR_RO(link_up_fail_ucw_limit_crossed);
static struct kobj_attribute link_autoneg_np_retry          = __ATTR_RO(link_autoneg_np_retry);
static struct kobj_attribute link_autoneg_attempt           = __ATTR_RO(link_autoneg_attempt);

static struct kobj_attribute link_cause_ucw               = __ATTR_RO(cause_ucw);
static struct kobj_attribute link_cause_lf                = __ATTR_RO(cause_lf);
static struct kobj_attribute link_cause_rf                = __ATTR_RO(cause_rf);
static struct kobj_attribute link_cause_down              = __ATTR_RO(cause_down);
static struct kobj_attribute link_cause_up_tries          = __ATTR_RO(cause_up_tries);
static struct kobj_attribute link_cause_autoneg_nomatch   = __ATTR_RO(cause_autoneg_nomatch);
static struct kobj_attribute link_cause_autoneg           = __ATTR_RO(cause_autoneg);
static struct kobj_attribute link_cause_config            = __ATTR_RO(cause_config);
static struct kobj_attribute link_cause_intr_enable       = __ATTR_RO(cause_intr_enable);
static struct kobj_attribute link_cause_timeout           = __ATTR_RO(cause_timeout);
static struct kobj_attribute link_cause_canceled          = __ATTR_RO(cause_canceled);
static struct kobj_attribute link_cause_unsupported_cable = __ATTR_RO(cause_unsupported_cable);
static struct kobj_attribute link_cause_command           = __ATTR_RO(cause_command);
static struct kobj_attribute link_cause_downshift         = __ATTR_RO(cause_downshift);
static struct kobj_attribute link_cause_llr_replay_max    = __ATTR_RO(cause_llr_replay_max);
static struct kobj_attribute link_cause_upshift           = __ATTR_RO(cause_upshift);
static struct kobj_attribute link_cause_autoneg_config    = __ATTR_RO(cause_autoneg_config);
static struct kobj_attribute link_cause_pcs_fault         = __ATTR_RO(cause_pcs_fault);
static struct kobj_attribute link_cause_serdes_pll        = __ATTR_RO(cause_serdes_pll);
static struct kobj_attribute link_cause_serdes_config     = __ATTR_RO(cause_serdes_config);
static struct kobj_attribute link_cause_serdes_signal     = __ATTR_RO(cause_serdes_signal);
static struct kobj_attribute link_cause_serdes_quality    = __ATTR_RO(cause_serdes_quality);
static struct kobj_attribute link_cause_no_media          = __ATTR_RO(cause_no_media);
static struct kobj_attribute link_cause_ccw               = __ATTR_RO(cause_ccw);
static struct kobj_attribute link_cause_high_temp         = __ATTR_RO(cause_high_temp);
static struct kobj_attribute link_cause_intr_register     = __ATTR_RO(cause_intr_register);
static struct kobj_attribute link_cause_media_error       = __ATTR_RO(cause_media_error);
static struct kobj_attribute link_cause_up_canceled       = __ATTR_RO(cause_up_canceled);
static struct kobj_attribute link_cause_unsupported_speed = __ATTR_RO(cause_unsupported_speed);
static struct kobj_attribute link_cause_ss200_cable       = __ATTR_RO(cause_ss200_cable);
static struct kobj_attribute link_cause_tx_lol            = __ATTR_RO(cause_tx_lol);
static struct kobj_attribute link_cause_rx_lol            = __ATTR_RO(cause_rx_lol);
static struct kobj_attribute link_cause_tx_los            = __ATTR_RO(cause_tx_los);
static struct kobj_attribute link_cause_rx_los            = __ATTR_RO(cause_rx_los);

static struct kobj_attribute link_an_cause_lp_caps_serdes_link_up_fail =
			     __ATTR_RO(an_cause_lp_caps_serdes_link_up_fail);
static struct kobj_attribute link_an_cause_lp_caps_not_complete        = __ATTR_RO(an_cause_lp_caps_not_complete);
static struct kobj_attribute link_an_cause_not_complete                = __ATTR_RO(an_cause_not_complete);
static struct kobj_attribute link_an_cause_test_caps_nomatch           = __ATTR_RO(an_cause_test_caps_nomatch);
static struct kobj_attribute link_an_cause_serdes_link_up_fail         = __ATTR_RO(an_cause_serdes_link_up_fail);
static struct kobj_attribute link_an_cause_bp_store_state_bad          = __ATTR_RO(an_cause_bp_store_state_bad);
static struct kobj_attribute link_an_cause_bp_store_lp_ability_not_set =
			     __ATTR_RO(an_cause_bp_store_lp_ability_not_set);
static struct kobj_attribute link_an_cause_bp_store_state_error        = __ATTR_RO(an_cause_bp_store_state_error);
static struct kobj_attribute link_an_cause_bp_store_bp_not_set         = __ATTR_RO(an_cause_bp_store_bp_not_set);
static struct kobj_attribute link_an_cause_bp_send_intr_enable_fail    =
			     __ATTR_RO(an_cause_bp_send_intr_enable_fail);
static struct kobj_attribute link_an_cause_np_store_state_bad          = __ATTR_RO(an_cause_np_store_state_bad);
static struct kobj_attribute link_an_cause_np_store_bp_set             = __ATTR_RO(an_cause_np_store_bp_set);
static struct kobj_attribute link_an_cause_np_check_state_bad          = __ATTR_RO(an_cause_np_check_state_bad);
static struct kobj_attribute link_an_cause_intr_state_invalid          = __ATTR_RO(an_cause_intr_state_invalid);
static struct kobj_attribute link_an_cause_intr_an_retry_np_send_fail  =
			     __ATTR_RO(an_cause_intr_an_retry_np_send_fail);
static struct kobj_attribute link_an_cause_intr_out_of_pages           = __ATTR_RO(an_cause_intr_out_of_pages);
static struct kobj_attribute link_an_cause_intr_np_send_fail           = __ATTR_RO(an_cause_intr_np_send_fail);
static struct kobj_attribute link_an_cause_pages_decode_fail           = __ATTR_RO(an_cause_pages_decode_fail);
static struct kobj_attribute link_an_cause_pages_decode_no_bp          = __ATTR_RO(an_cause_pages_decode_no_bp);
static struct kobj_attribute link_an_cause_pages_decode_oui_invalid    =
			     __ATTR_RO(an_cause_pages_decode_oui_invalid);

static struct attribute *link_counters_attrs[] = {
	&link_up_cmd.attr,
	&link_up_retry.attr,
	&link_up.attr,
	&link_up_fail.attr,
	&link_down_cmd.attr,
	&link_down.attr,
	&link_up_cancel_cmd.attr,
	&link_up_canceled.attr,
	&link_reset_cmd.attr,
	&link_fault_async.attr,
	&link_ccw_warn_crossed.attr,
	&link_ucw_warn_crossed.attr,
	&link_down_ccw_limit_crossed.attr,
	&link_down_ucw_limit_crossed.attr,
	&link_down_ccw_cause.attr,
	&link_down_ucw_cause.attr,
	&link_up_fail_ccw_limit_crossed.attr,
	&link_up_fail_ucw_limit_crossed.attr,
	&link_autoneg_np_retry.attr,
	&link_autoneg_attempt.attr,
	&link_cause_ucw.attr,
	&link_cause_lf.attr,
	&link_cause_rf.attr,
	&link_cause_down.attr,
	&link_cause_up_tries.attr,
	&link_cause_autoneg_nomatch.attr,
	&link_cause_autoneg.attr,
	&link_cause_config.attr,
	&link_cause_intr_enable.attr,
	&link_cause_timeout.attr,
	&link_cause_canceled.attr,
	&link_cause_unsupported_cable.attr,
	&link_cause_command.attr,
	&link_cause_downshift.attr,
	&link_cause_llr_replay_max.attr,
	&link_cause_upshift.attr,
	&link_cause_autoneg_config.attr,
	&link_cause_pcs_fault.attr,
	&link_cause_serdes_pll.attr,
	&link_cause_serdes_config.attr,
	&link_cause_serdes_signal.attr,
	&link_cause_serdes_quality.attr,
	&link_cause_no_media.attr,
	&link_cause_ccw.attr,
	&link_cause_high_temp.attr,
	&link_cause_intr_register.attr,
	&link_cause_media_error.attr,
	&link_cause_up_canceled.attr,
	&link_cause_unsupported_speed.attr,
	&link_cause_ss200_cable.attr,
	&link_cause_tx_lol.attr,
	&link_cause_rx_lol.attr,
	&link_cause_tx_los.attr,
	&link_cause_rx_los.attr,
	&link_an_cause_lp_caps_serdes_link_up_fail.attr,
	&link_an_cause_lp_caps_not_complete.attr,
	&link_an_cause_not_complete.attr,
	&link_an_cause_test_caps_nomatch.attr,
	&link_an_cause_serdes_link_up_fail.attr,
	&link_an_cause_bp_store_state_bad.attr,
	&link_an_cause_bp_store_lp_ability_not_set.attr,
	&link_an_cause_bp_store_state_error.attr,
	&link_an_cause_bp_store_bp_not_set.attr,
	&link_an_cause_bp_send_intr_enable_fail.attr,
	&link_an_cause_np_store_state_bad.attr,
	&link_an_cause_np_store_bp_set.attr,
	&link_an_cause_np_check_state_bad.attr,
	&link_an_cause_intr_state_invalid.attr,
	&link_an_cause_intr_an_retry_np_send_fail.attr,
	&link_an_cause_intr_out_of_pages.attr,
	&link_an_cause_intr_np_send_fail.attr,
	&link_an_cause_pages_decode_fail.attr,
	&link_an_cause_pages_decode_no_bp.attr,
	&link_an_cause_pages_decode_oui_invalid.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_counters);

static struct kobj_type link_counters = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_counters_groups,
};

int sl_sysfs_link_counters_create(struct sl_ctrl_link *ctrl_link)
{
	int rtn;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link counters create");

	rtn = kobject_init_and_add(&ctrl_link->counters_kobj, &link_counters, &ctrl_link->kobj, "counters");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link counters create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_link->counters_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_counters_delete(struct sl_ctrl_link *ctrl_link)
{
	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME, "link counters delete");
	kobject_put(&ctrl_link->counters_kobj);
}
