// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_sysfs.h"
#include "sl_log.h"
#include "sl_ctrl_llr.h"
#include "sl_ctrl_llr_counters.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t llr_setup_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_SETUP_CMD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr setup cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_setup_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_SETUP, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr setup show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_setup_timeout_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_SETUP_TIMEOUT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr setup timeout show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_setup_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_SETUP_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr setup fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_configured_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_CONFIGURED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr configured show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_start_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_START_CMD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr start cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_running_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_RUNNING, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr running show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_start_timeout_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_START_TIMEOUT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr start timeout show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_start_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_START_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr start fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_stop_cmd_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_STOP_CMD, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr stop cmd show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t llr_stop_fail_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_counter_get(ctrl_llr, LLR_STOP_FAIL, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr stop fail show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_setup_config_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_cause_counter_get(ctrl_llr, LLR_CAUSE_SETUP_CONFIG, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr cause setup config show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_setup_intr_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_cause_counter_get(ctrl_llr, LLR_CAUSE_SETUP_INTR_ENABLE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr cause setup intr enable show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_setup_timeout_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_cause_counter_get(ctrl_llr, LLR_CAUSE_SETUP_TIMEOUT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr cause setup timeout show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_start_intr_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_cause_counter_get(ctrl_llr, LLR_CAUSE_START_INTR_ENABLE, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr cause start intr enable show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_start_timeout_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_cause_counter_get(ctrl_llr, LLR_CAUSE_START_TIMEOUT, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr cause start timeout show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_command_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_cause_counter_get(ctrl_llr, LLR_CAUSE_COMMAND, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr cause command show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t cause_canceled_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr *ctrl_llr;
	u32                 counter;
	u32                 rtn;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, counters_kobj);

	rtn = sl_ctrl_llr_cause_counter_get(ctrl_llr, LLR_CAUSE_CANCELED, &counter);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr cause canceled show (counter = %u)", counter);

	return scnprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static struct kobj_attribute llr_setup_cmd     = __ATTR_RO(llr_setup_cmd);
static struct kobj_attribute llr_setup         = __ATTR_RO(llr_setup);
static struct kobj_attribute llr_setup_timeout = __ATTR_RO(llr_setup_timeout);
static struct kobj_attribute llr_setup_fail    = __ATTR_RO(llr_setup_fail);
static struct kobj_attribute llr_configured    = __ATTR_RO(llr_configured);
static struct kobj_attribute llr_start_cmd     = __ATTR_RO(llr_start_cmd);
static struct kobj_attribute llr_running       = __ATTR_RO(llr_running);
static struct kobj_attribute llr_start_timeout = __ATTR_RO(llr_start_timeout);
static struct kobj_attribute llr_start_fail    = __ATTR_RO(llr_start_fail);
static struct kobj_attribute llr_stop_cmd      = __ATTR_RO(llr_stop_cmd);
static struct kobj_attribute llr_stop_fail     = __ATTR_RO(llr_stop_fail);

static struct kobj_attribute llr_cause_setup_config      = __ATTR_RO(cause_setup_config);
static struct kobj_attribute llr_cause_setup_intr_enable = __ATTR_RO(cause_setup_intr_enable);
static struct kobj_attribute llr_cause_setup_timeout     = __ATTR_RO(cause_setup_timeout);
static struct kobj_attribute llr_cause_start_intr_enable = __ATTR_RO(cause_start_intr_enable);
static struct kobj_attribute llr_cause_start_timeout     = __ATTR_RO(cause_start_timeout);
static struct kobj_attribute llr_cause_command           = __ATTR_RO(cause_command);
static struct kobj_attribute llr_cause_canceled          = __ATTR_RO(cause_canceled);

static struct attribute *llr_counters_attrs[] = {
	&llr_setup_cmd.attr,
	&llr_setup.attr,
	&llr_setup_timeout.attr,
	&llr_setup_fail.attr,
	&llr_configured.attr,
	&llr_start_cmd.attr,
	&llr_running.attr,
	&llr_start_timeout.attr,
	&llr_start_fail.attr,
	&llr_stop_cmd.attr,
	&llr_stop_fail.attr,
	&llr_cause_setup_config.attr,
	&llr_cause_setup_intr_enable.attr,
	&llr_cause_setup_timeout.attr,
	&llr_cause_start_intr_enable.attr,
	&llr_cause_start_timeout.attr,
	&llr_cause_command.attr,
	&llr_cause_canceled.attr,
	NULL
};
ATTRIBUTE_GROUPS(llr_counters);

static struct kobj_type llr_counters = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = llr_counters_groups,
};

int sl_sysfs_llr_counters_create(struct sl_ctrl_llr *ctrl_llr, struct kobject *parent_kobj)
{
	int rtn;

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr counters create");

	rtn = kobject_init_and_add(&ctrl_llr->counters_kobj, &llr_counters, parent_kobj, "counters");
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr counters create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_llr->counters_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_llr_counters_delete(struct sl_ctrl_llr *ctrl_llr)
{
	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr counters delete");
	kobject_put(&ctrl_llr->counters_kobj);
}
