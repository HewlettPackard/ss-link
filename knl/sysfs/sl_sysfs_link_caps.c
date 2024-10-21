// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_core_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_link_priv.h"
#include "sl_test_common.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t tech_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	int                  idx;
	char                 output[80];

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);
	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	core_lgrp = core_link->core_lgrp;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"tech map show (link = 0x%p, map = 0x%X)",
		core_link, core_lgrp->link_caps[core_link->num].tech_map);

	idx = 0;
	if (is_flag_set(core_lgrp->link_caps[core_link->num].tech_map, SL_LGRP_CONFIG_TECH_CK_400G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_400G));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].tech_map, SL_LGRP_CONFIG_TECH_CK_200G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_200G));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].tech_map, SL_LGRP_CONFIG_TECH_CK_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CK_100G));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].tech_map, SL_LGRP_CONFIG_TECH_BS_200G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_BS_200G));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].tech_map, SL_LGRP_CONFIG_TECH_CD_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CD_100G));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].tech_map, SL_LGRP_CONFIG_TECH_CD_50G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_CD_50G));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].tech_map, SL_LGRP_CONFIG_TECH_BJ_100G))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_tech_str(SL_LGRP_CONFIG_TECH_BJ_100G));

	if (idx == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t pause_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	int                  idx;
	char                 output[20];

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);
	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	core_lgrp = core_link->core_lgrp;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"pause map show (link = 0x%p, map = 0x%X)",
		core_link, core_lgrp->link_caps[core_link->num].pause_map);

	idx = 0;
	if (is_flag_set(core_lgrp->link_caps[core_link->num].pause_map, SL_LINK_CONFIG_PAUSE_ASYM))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_pause_str(SL_LINK_CONFIG_PAUSE_ASYM));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].pause_map, SL_LINK_CONFIG_PAUSE_SYM))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_pause_str(SL_LINK_CONFIG_PAUSE_SYM));

	if (idx == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t fec_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	int                  idx;
	char                 output[20];

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);
	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	core_lgrp = core_link->core_lgrp;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"fec map show (link = 0x%p, map = 0x%X)",
		core_link, core_lgrp->link_caps[core_link->num].fec_map);

	idx = 0;
	if (is_flag_set(core_lgrp->link_caps[core_link->num].fec_map, SL_LGRP_CONFIG_FEC_RS_LL))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_fec_str(SL_LGRP_CONFIG_FEC_RS_LL));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].fec_map, SL_LGRP_CONFIG_FEC_RS))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_lgrp_config_fec_str(SL_LGRP_CONFIG_FEC_RS));

	if (idx == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t hpe_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	int                  idx;
	char                 output[80];

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);
	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	core_lgrp = core_link->core_lgrp;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"hpe map show (link = 0x%p, map = 0x%X)",
		core_link, core_lgrp->link_caps[core_link->num].hpe_map);

	idx = 0;
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_LINKTRAIN));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_PRECODING))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_PRECODING));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_PCAL))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_PCAL));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_R3))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R3));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_R2))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R2));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_C3))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R3));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_C2))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R2));
	if (is_flag_set(core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_LLR))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_LLR));

	if (idx == 0)
		return scnprintf(buf, PAGE_SIZE, "none\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static struct kobj_attribute tech_map  = __ATTR_RO(tech_map);
static struct kobj_attribute fec_map   = __ATTR_RO(fec_map);
static struct kobj_attribute pause_map = __ATTR_RO(pause_map);
static struct kobj_attribute hpe_map   = __ATTR_RO(hpe_map);

static struct attribute *link_caps_attrs[] = {
	&tech_map.attr,
	&fec_map.attr,
	&pause_map.attr,
	&hpe_map.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_caps);

static struct kobj_type link_caps = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_caps_groups,
};

int sl_sysfs_link_caps_create(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link caps create (num = %u)", ctl_link->num);

	rtn = kobject_init_and_add(&ctl_link->caps_kobj, &link_caps, &ctl_link->kobj, "caps");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link caps create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_link->caps_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_link_caps_delete(struct sl_ctl_link *ctl_link)
{
	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME, "link caps delete (num = %u)", ctl_link->num);

	kobject_put(&ctl_link->caps_kobj);
}
