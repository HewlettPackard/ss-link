// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

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
	u32                  tech_map;
	u32                  link_state;

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = core_link->core_lgrp;

	tech_map = core_lgrp->link_caps[core_link->num].tech_map;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"tech map show (link = 0x%p, map = 0x%X)", core_link, tech_map);

	sl_core_link_state_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &link_state);

	if ((tech_map == 0) ||
		((link_state != SL_CORE_LINK_STATE_UP) &&
		 (link_state != SL_CORE_LINK_STATE_GOING_UP) &&
		 (link_state != SL_CORE_LINK_STATE_AN)) ||
		!is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE))
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

static ssize_t pause_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	int                  idx;
	char                 output[20];
	u32                  pause_map;
	u32                  link_state;

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = core_link->core_lgrp;

	pause_map = core_lgrp->link_caps[core_link->num].pause_map;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"pause map show (link = 0x%p, map = 0x%X)", core_link, pause_map);

	sl_core_link_state_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &link_state);

	if ((pause_map == 0) ||
		((link_state != SL_CORE_LINK_STATE_UP) &&
		 (link_state != SL_CORE_LINK_STATE_GOING_UP) &&
		 (link_state != SL_CORE_LINK_STATE_AN)) ||
		!is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE))
		return scnprintf(buf, PAGE_SIZE, "none\n");

	idx = 0;
	if (is_flag_set(pause_map, SL_LINK_CONFIG_PAUSE_ASYM))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_pause_str(SL_LINK_CONFIG_PAUSE_ASYM));
	if (is_flag_set(pause_map, SL_LINK_CONFIG_PAUSE_SYM))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_pause_str(SL_LINK_CONFIG_PAUSE_SYM));
	output[idx - 1] = '\0';

	return scnprintf(buf, PAGE_SIZE, "%s\n", output);
}

static ssize_t fec_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	int                  idx;
	char                 output[20];
	u32                  fec_map;
	u32                  link_state;

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = core_link->core_lgrp;

	fec_map = core_lgrp->link_caps[core_link->num].fec_map;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"fec map show (link = 0x%p, map = 0x%X)", core_link, fec_map);

	sl_core_link_state_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &link_state);

	if ((fec_map == 0) ||
		((link_state != SL_CORE_LINK_STATE_UP) &&
		 (link_state != SL_CORE_LINK_STATE_GOING_UP) &&
		 (link_state != SL_CORE_LINK_STATE_AN)) ||
		!is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE))
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

static ssize_t hpe_map_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link  *ctl_link;
	struct sl_core_link *core_link;
	struct sl_core_lgrp *core_lgrp;
	int                  idx;
	char                 output[80];
	u32                  hpe_map;
	u32                  link_state;

	ctl_link = container_of(kobj, struct sl_ctl_link, caps_kobj);

	core_link = sl_core_link_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = core_link->core_lgrp;

	hpe_map = core_lgrp->link_caps[core_link->num].hpe_map;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"hpe map show (link = 0x%p, map = 0x%X)", core_link, hpe_map);

	sl_core_link_state_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &link_state);

	if ((hpe_map == 0) ||
		((link_state != SL_CORE_LINK_STATE_UP) &&
		 (link_state != SL_CORE_LINK_STATE_GOING_UP) &&
		 (link_state != SL_CORE_LINK_STATE_AN)) ||
		!is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE))
		return scnprintf(buf, PAGE_SIZE, "none\n");

	idx = 0;
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_LINKTRAIN));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_PRECODING))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_PRECODING));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_PCAL))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_PCAL));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_R3))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R3));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_R2))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R2));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_C3))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R3));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_C2))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_R2));
	if (is_flag_set(hpe_map, SL_LINK_CONFIG_HPE_LLR))
		idx += snprintf(output + idx, sizeof(output) - idx, "%s ",
			sl_link_config_hpe_str(SL_LINK_CONFIG_HPE_LLR));
	output[idx - 1] = '\0';

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
