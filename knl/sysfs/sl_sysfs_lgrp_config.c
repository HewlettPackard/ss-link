// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t mfs_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"mfs show (lgrp = 0x%p, mfs = %u)",
		ctl_lgrp, ctl_lgrp->config.mfs);

	return scnprintf(buf, PAGE_SIZE, "%u\n", ctl_lgrp->config.mfs);
}

static ssize_t furcation_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"furcation show (lgrp = 0x%p, furcation = %u %s)",
		ctl_lgrp, ctl_lgrp->config.furcation, sl_lgrp_furcation_str(ctl_lgrp->config.furcation));

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		sl_lgrp_furcation_str(ctl_lgrp->config.furcation));
}

static ssize_t fec_mode_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"fec mode show (lgrp = 0x%p, mode = %u %s)",
		ctl_lgrp, ctl_lgrp->config.fec_mode, sl_lgrp_fec_mode_str(ctl_lgrp->config.fec_mode));

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		sl_lgrp_fec_mode_str(ctl_lgrp->config.fec_mode));
}

static ssize_t tech_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	int                 idx;
	char                output[50];

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"tech map show (lgrp = 0x%p, map = 0x%X)", ctl_lgrp, ctl_lgrp->config.tech_map);

	idx = 0;
	if (is_flag_set(ctl_lgrp->config.tech_map, SL_LGRP_CONFIG_TECH_CK_400G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_400G));
	if (is_flag_set(ctl_lgrp->config.tech_map, SL_LGRP_CONFIG_TECH_CK_200G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_200G));
	if (is_flag_set(ctl_lgrp->config.tech_map, SL_LGRP_CONFIG_TECH_CK_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_100G));
	if (is_flag_set(ctl_lgrp->config.tech_map, SL_LGRP_CONFIG_TECH_BS_200G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_BS_200G));
	if (is_flag_set(ctl_lgrp->config.tech_map, SL_LGRP_CONFIG_TECH_CD_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CD_100G));
	if (is_flag_set(ctl_lgrp->config.tech_map, SL_LGRP_CONFIG_TECH_CD_50G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CD_50G));
	if (is_flag_set(ctl_lgrp->config.tech_map, SL_LGRP_CONFIG_TECH_BJ_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_BJ_100G));

	if (idx == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t fec_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	int                 idx;
	char                output[20];

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"fec map show (lgrp = 0x%p, map = 0x%X)", ctl_lgrp, ctl_lgrp->config.fec_map);

	idx = 0;
	if (is_flag_set(ctl_lgrp->config.fec_map, SL_LGRP_CONFIG_FEC_RS_LL))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_fec_str(SL_LGRP_CONFIG_FEC_RS_LL));
	if (is_flag_set(ctl_lgrp->config.fec_map, SL_LGRP_CONFIG_FEC_RS))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_fec_str(SL_LGRP_CONFIG_FEC_RS));

	if (idx == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

// FIXME: if/when options get added, they go here

static struct kobj_attribute lgrp_mfs       = __ATTR_RO(mfs);
static struct kobj_attribute lgrp_furcation = __ATTR_RO(furcation);
static struct kobj_attribute lgrp_fec_mode  = __ATTR_RO(fec_mode);
static struct kobj_attribute lgrp_tech_map  = __ATTR_RO(tech_map);
static struct kobj_attribute lgrp_fec_map   = __ATTR_RO(fec_map);

static ssize_t err_trace_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"err trace enable show (lgrp = 0x%p, err trace enable = %u)",
		ctl_lgrp, ctl_lgrp->err_trace_enable ? 1 : 0);

	return scnprintf(buf, PAGE_SIZE, "%u\n", ctl_lgrp->err_trace_enable ? 1 : 0);
}

static ssize_t err_trace_enable_store(struct kobject *kobj, struct kobj_attribute *kattr, const char *buf, size_t count)
{
	int                   rtn;
	struct sl_ctl_lgrp   *ctl_lgrp;
	struct sl_core_lgrp  *core_lgrp;
	struct sl_media_lgrp *media_lgrp;
	u8                    val;

	rtn = kstrtou8(buf, 0, &val);
	if (rtn)
		return rtn;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"err trace enable store (lgrp = 0x%p, val = %u)",
		ctl_lgrp, val);

	core_lgrp = sl_core_lgrp_get(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num);

	media_lgrp = sl_media_lgrp_get(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num);

	if (val == 0) {
		ctl_lgrp->err_trace_enable   = false;
		core_lgrp->err_trace_enable  = false;
		media_lgrp->err_trace_enable = false;
	} else {
		ctl_lgrp->err_trace_enable   = true;
		core_lgrp->err_trace_enable  = true;
		media_lgrp->err_trace_enable = true;
	}

	return count;
}
// FIXME: put this in it's own node?
static struct kobj_attribute lgrp_err_trace_enable = __ATTR_RW(err_trace_enable);

static ssize_t warn_trace_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"warn trace enable show (lgrp = 0x%p, warn trace enable = %u)",
		ctl_lgrp, ctl_lgrp->warn_trace_enable ? 1 : 0);

	return scnprintf(buf, PAGE_SIZE, "%u\n", ctl_lgrp->warn_trace_enable ? 1 : 0);
}

static ssize_t warn_trace_enable_store(struct kobject *kobj, struct kobj_attribute *kattr,
	const char *buf, size_t count)
{
	int                   rtn;
	struct sl_ctl_lgrp   *ctl_lgrp;
	struct sl_core_lgrp  *core_lgrp;
	struct sl_media_lgrp *media_lgrp;
	u8                    val;

	rtn = kstrtou8(buf, 0, &val);
	if (rtn)
		return rtn;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"warn trace enable store (lgrp = 0x%p, val = %u)",
		ctl_lgrp, val);

	core_lgrp = sl_core_lgrp_get(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num);

	media_lgrp = sl_media_lgrp_get(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num);

	if (val == 0) {
		ctl_lgrp->warn_trace_enable   = false;
		core_lgrp->warn_trace_enable  = false;
		media_lgrp->warn_trace_enable = false;
	} else {
		ctl_lgrp->warn_trace_enable   = true;
		core_lgrp->warn_trace_enable  = true;
		media_lgrp->warn_trace_enable = true;
	}

	return count;
}
static struct kobj_attribute lgrp_warn_trace_enable = __ATTR_RW(warn_trace_enable);

static struct attribute *lgrp_config_attrs[] = {
	&lgrp_mfs.attr,
	&lgrp_furcation.attr,
	&lgrp_fec_mode.attr,
	&lgrp_tech_map.attr,
	&lgrp_fec_map.attr,
	&lgrp_err_trace_enable.attr,
	&lgrp_warn_trace_enable.attr,
	NULL
};
ATTRIBUTE_GROUPS(lgrp_config);

static struct kobj_type config_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = lgrp_config_groups,
};

int sl_sysfs_lgrp_config_create(struct sl_ctl_lgrp *ctl_lgrp)
{
	int rtn;

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp config create (lgrp = 0x%p)", ctl_lgrp);

	rtn = kobject_init_and_add(&(ctl_lgrp->config_kobj), &config_info, ctl_lgrp->parent_kobj, "config");
	if (rtn) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp config create failed [%d]", rtn);
		kobject_put(&(ctl_lgrp->config_kobj));
		return -ENOMEM;
	}

	return 0;
}

void sl_sysfs_lgrp_config_delete(struct sl_ctl_lgrp *ctl_lgrp)
{
	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp config delete (lgrp = 0x%p)", ctl_lgrp);

	kobject_put(&(ctl_lgrp->config_kobj));
}
