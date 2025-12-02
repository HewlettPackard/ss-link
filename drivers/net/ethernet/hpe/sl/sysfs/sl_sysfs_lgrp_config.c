// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/hpe/sl/sl_lgrp.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "data/sl_ctrl_data_lgrp.h"
#include "data/sl_ctrl_data_lgrp_trace.h"

//FIXME: move somewhere common
#define is_flag_set(_flags, _flag) ({              \
		typeof(_flags) __flags = (_flags); \
		typeof(_flag)  __flag  = (_flag);  \
						   \
		((__flags & __flag) == __flag);    \
})

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t mfs_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u32                  mfs;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_mfs_get(ctrl_lgrp, &mfs);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "mfs show (mfs = %u)", mfs);

	return scnprintf(buf, PAGE_SIZE, "%u\n", mfs);
}

static ssize_t furcation_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u32                  furcation;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_furcation_get(ctrl_lgrp, &furcation);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "furcation show (furcation = %u %s)",
		   furcation, sl_lgrp_furcation_str(furcation));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_lgrp_furcation_str(furcation));
}

static ssize_t fec_mode_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u32                  fec_mode;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_fec_mode_get(ctrl_lgrp, &fec_mode);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "fec mode show (mode = %u %s)",
		   fec_mode, sl_lgrp_fec_mode_str(fec_mode));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_lgrp_fec_mode_str(fec_mode));
}

static ssize_t tech_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	int                  idx;
	char                 output[50];
	u32                  tech_map;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_tech_map_get(ctrl_lgrp, &tech_map);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "tech map show (map = 0x%X)", tech_map);

	if (tech_map == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	idx = 0;
	if (is_flag_set(tech_map, SL_LGRP_CONFIG_TECH_CK_400G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_400G));
	if (is_flag_set(tech_map, SL_LGRP_CONFIG_TECH_CK_200G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_200G));
	if (is_flag_set(tech_map, SL_LGRP_CONFIG_TECH_CK_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_100G));
	if (is_flag_set(tech_map, SL_LGRP_CONFIG_TECH_BS_200G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_BS_200G));
	if (is_flag_set(tech_map, SL_LGRP_CONFIG_TECH_CD_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CD_100G));
	if (is_flag_set(tech_map, SL_LGRP_CONFIG_TECH_CD_50G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CD_50G));
	if (is_flag_set(tech_map, SL_LGRP_CONFIG_TECH_BJ_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_BJ_100G));
	output[idx - 1] = '\0';

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t fec_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	int                  idx;
	char                 output[20];
	u32                  fec_map;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_fec_map_get(ctrl_lgrp, &fec_map);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "fec map show (map = 0x%X)", fec_map);

	if (fec_map == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	idx = 0;
	if (is_flag_set(fec_map, SL_LGRP_CONFIG_FEC_RS_LL))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_fec_str(SL_LGRP_CONFIG_FEC_RS_LL));
	if (is_flag_set(fec_map, SL_LGRP_CONFIG_FEC_RS))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_fec_str(SL_LGRP_CONFIG_FEC_RS));
	output[idx - 1] = '\0';

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t link_type_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u32                  options;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_options_get(ctrl_lgrp, &options);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	if (options & SL_LGRP_CONFIG_OPT_FABRIC)
		return scnprintf(buf, PAGE_SIZE, "fabric\n");

	return scnprintf(buf, PAGE_SIZE, "edge\n");
}

static struct kobj_attribute lgrp_mfs       = __ATTR_RO(mfs);
static struct kobj_attribute lgrp_furcation = __ATTR_RO(furcation);
static struct kobj_attribute lgrp_fec_mode  = __ATTR_RO(fec_mode);
static struct kobj_attribute lgrp_tech_map  = __ATTR_RO(tech_map);
static struct kobj_attribute lgrp_fec_map   = __ATTR_RO(fec_map);
static struct kobj_attribute lgrp_link_type = __ATTR_RO(link_type);

static ssize_t err_trace_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	bool                 is_err_trace_enabled;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	//FIXME: This checks ctrl, core, and media layers
	rtn = sl_ctrl_data_lgrp_is_err_trace_enabled(ctrl_lgrp, &is_err_trace_enabled);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		   "err trace enable show (is_err_trace_enabled = %u)", is_err_trace_enabled);

	return scnprintf(buf, PAGE_SIZE, "%u\n", is_err_trace_enabled);
}

static ssize_t err_trace_enable_store(struct kobject *kobj, struct kobj_attribute *kattr, const char *buf, size_t count)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u8                   val;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = kstrtou8(buf, 0, &val);
	if (rtn) {
		sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			   "err trace enable store failed [%d]", rtn);
		return count;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "err trace enable store (val = %u)", val);

	//FIXME: This sets ctrl, core, and media layers
	rtn = sl_ctrl_data_lgrp_err_trace_enable_set(ctrl_lgrp, val);
	if (rtn) {
		sl_log_err_trace(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				 "err trace enable store failed [%d]", rtn);
		return count;
	}

	return count;
}
// FIXME: put this in it's own node?
static struct kobj_attribute lgrp_err_trace_enable = __ATTR_RW(err_trace_enable);

static ssize_t warn_trace_enable_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	bool                 is_warn_trace_enabled;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_is_warn_trace_enabled(ctrl_lgrp, &is_warn_trace_enabled);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "warn trace enable show (is_warn_trace_enabled = %u)",
		   is_warn_trace_enabled);

	return scnprintf(buf, PAGE_SIZE, "%u\n", is_warn_trace_enabled);
}

static ssize_t warn_trace_enable_store(struct kobject *kobj, struct kobj_attribute *kattr,
	const char *buf, size_t count)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u8                   val;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = kstrtou8(buf, 0, &val);
	if (rtn) {
		sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			   "warn trace enable store failed [%d]", rtn);
		return count;
	}

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "warn trace enable store (val = %u)", val);

	rtn = sl_ctrl_data_lgrp_warn_trace_set(ctrl_lgrp, val);
	if (rtn) {
		sl_log_err_trace(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
				 "warn trace enable store failed [%d]", rtn);
		return count;
	}

	return count;
}
static struct kobj_attribute lgrp_warn_trace_enable = __ATTR_RW(warn_trace_enable);

static ssize_t fabric_link_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u32                  options;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_options_get(ctrl_lgrp, &options);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		   "fabric link show (fabric link = %s)",
		   (options & SL_LGRP_CONFIG_OPT_FABRIC) ? "enabled" : "disabled");

	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 (options & SL_LGRP_CONFIG_OPT_FABRIC) ? "enabled" : "disabled");
}
static struct kobj_attribute lgrp_fabric_link = __ATTR_RO(fabric_link);

static ssize_t r1_partner_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u32                  options;

	ctrl_lgrp = container_of(kobj, struct sl_ctrl_lgrp, config_kobj);

	rtn = sl_ctrl_data_lgrp_options_get(ctrl_lgrp, &options);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		   "r1 partner show (r1 partner = %s)",
		   (options & SL_LGRP_CONFIG_OPT_R1) ? "enabled" : "disabled");

	return scnprintf(buf, PAGE_SIZE, "%s\n", (options & SL_LGRP_CONFIG_OPT_R1) ? "enabled" : "disabled");
}
static struct kobj_attribute lgrp_r1_partner  = __ATTR_RO(r1_partner);

static struct attribute *lgrp_config_attrs[] = {
	&lgrp_mfs.attr,
	&lgrp_furcation.attr,
	&lgrp_fec_mode.attr,
	&lgrp_tech_map.attr,
	&lgrp_fec_map.attr,
	&lgrp_link_type.attr,
	&lgrp_err_trace_enable.attr,
	&lgrp_warn_trace_enable.attr,
	&lgrp_fabric_link.attr,
	&lgrp_r1_partner.attr,
	NULL
};
ATTRIBUTE_GROUPS(lgrp_config);

static struct kobj_type config_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = lgrp_config_groups,
};

int sl_sysfs_lgrp_config_create(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp config create (lgrp = 0x%p)", ctrl_lgrp);

	rtn = kobject_init_and_add(&(ctrl_lgrp->config_kobj), &config_info, ctrl_lgrp->parent_kobj, "config");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp config create failed [%d]", rtn);
		kobject_put(&(ctrl_lgrp->config_kobj));
		return -ENOMEM;
	}

	return 0;
}

void sl_sysfs_lgrp_config_delete(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp config delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&(ctrl_lgrp->config_kobj));
}
