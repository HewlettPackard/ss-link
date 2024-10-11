// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

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
		"mfs show (lgrp = 0x%p)", ctl_lgrp);

	return scnprintf(buf, PAGE_SIZE, "%u\n", ctl_lgrp->config.mfs);
}

static ssize_t furcation_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"furcation show (lgrp = 0x%p)", ctl_lgrp);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		sl_lgrp_furcation_str(ctl_lgrp->config.furcation));
}

static ssize_t caps_tech_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	int                 idx;
	char                output[50];

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"caps tech show (lgrp = 0x%p, map = 0x%X)", ctl_lgrp, ctl_lgrp->config.tech_map);

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

static ssize_t caps_fec_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	int                 idx;
	char                output[20];

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"caps fec show (lgrp = 0x%p, map = 0x%X)", ctl_lgrp, ctl_lgrp->config.fec_map);

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

static struct kobj_attribute lgrp_mfs       = __ATTR_RO(mfs);
static struct kobj_attribute lgrp_furcation = __ATTR_RO(furcation);
static struct kobj_attribute lgrp_caps_tech = __ATTR_RO(caps_tech);
static struct kobj_attribute lgrp_caps_fec  = __ATTR_RO(caps_fec);

static ssize_t err_trace_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

	return scnprintf(buf, PAGE_SIZE, "%u\n", ctl_lgrp->err_trace_enable ? 1 : 0);
}

static ssize_t err_trace_enable_store(struct kobject *kobj, struct kobj_attribute *kattr, const char *buf, size_t count)
{
	int                   rtn;
	struct sl_ctl_lgrp   *ctl_lgrp;
	struct sl_core_lgrp  *core_lgrp;
	struct sl_media_lgrp *media_lgrp;
	u8                    val;

	rtn = kstrtou8(buf, 10, &val);
	if (rtn)
		return rtn;

	ctl_lgrp = container_of(kobj, struct sl_ctl_lgrp, config_kobj);

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

static struct attribute *lgrp_attrs[] = {
	&lgrp_mfs.attr,
	&lgrp_furcation.attr,
	&lgrp_caps_tech.attr,
	&lgrp_caps_fec.attr,
	&lgrp_err_trace_enable.attr,
	NULL
};
ATTRIBUTE_GROUPS(lgrp);

static struct kobj_type config_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = lgrp_groups,
};

int sl_sysfs_lgrp_create(struct sl_ctl_lgrp *ctl_lgrp)
{
	int rtn;

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp create (lgrp = 0x%p)", ctl_lgrp);

	if (!ctl_lgrp->parent_kobj) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp create missing parent");
		return -EBADRQC;
	}

	rtn = kobject_init_and_add(&(ctl_lgrp->config_kobj), &config_info, ctl_lgrp->parent_kobj, "config");
	if (rtn) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "config create failed [%d]", rtn);
		kobject_put(&(ctl_lgrp->config_kobj));
		return -ENOMEM;
	}

	rtn = sl_sysfs_serdes_create(ctl_lgrp);
	if (rtn) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "serdes create failed [%d]", rtn);
		kobject_put(&(ctl_lgrp->config_kobj));
		return -ENOMEM;
	}

	rtn = sl_sysfs_media_create(ctl_lgrp);
	if (rtn) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME, "media create failed [%d]", rtn);
		kobject_put(&(ctl_lgrp->config_kobj));
		return -ENOMEM;
	}

	return 0;
}

void sl_sysfs_lgrp_delete(struct sl_ctl_lgrp *ctl_lgrp)
{
	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp delete (lgrp = 0x%p)", ctl_lgrp);

	if (!ctl_lgrp->parent_kobj)
		return;

	sl_sysfs_media_delete(ctl_lgrp);

	sl_sysfs_serdes_delete(ctl_lgrp);

	kobject_put(&(ctl_lgrp->config_kobj));
}
