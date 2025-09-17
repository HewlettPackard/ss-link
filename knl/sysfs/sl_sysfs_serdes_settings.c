// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include <linux/sl_media.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "sl_sysfs_serdes_settings.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t pre1_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	s16                              pre1;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_pre1_get(core_lgrp, lane_kobj->asic_lane_num, &pre1);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "settings pre1 show (pre1 = %d)", pre1);

	return scnprintf(buf, PAGE_SIZE, "%d\n", pre1);
}

static ssize_t pre2_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	s16                              pre2;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_pre2_get(core_lgrp, lane_kobj->asic_lane_num, &pre2);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "settings pre2 show (pre2 = %d)", pre2);

	return scnprintf(buf, PAGE_SIZE, "%d\n", pre2);
}

static ssize_t pre3_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	s16                              pre3;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_pre3_get(core_lgrp, lane_kobj->asic_lane_num, &pre3);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "settings pre3 show (pre3 = %d)", pre3);

	return scnprintf(buf, PAGE_SIZE, "%d\n", pre3);
}

static ssize_t cursor_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	s16                              cursor;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_cursor_get(core_lgrp, lane_kobj->asic_lane_num, &cursor);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "settings cursor show (cursor = %d)", cursor);

	return scnprintf(buf, PAGE_SIZE, "%d\n", cursor);
}

static ssize_t post1_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	s16                              post1;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_post1_get(core_lgrp, lane_kobj->asic_lane_num, &post1);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "settings post1 show (post1 = %d)", post1);

	return scnprintf(buf, PAGE_SIZE, "%d\n", post1);
}

static ssize_t post2_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	s16                              post2;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_post2_get(core_lgrp, lane_kobj->asic_lane_num, &post2);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME, "settings post2 show (post2 = %d)", post2);

	return scnprintf(buf, PAGE_SIZE, "%d\n", post2);
}

static ssize_t media_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_link             *core_link;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"settings media show (media = %d)", core_link->serdes.media_serdes_settings.media);
// FIXME: get from hardware?
	return scnprintf(buf, PAGE_SIZE, "%d\n", core_link->serdes.media_serdes_settings.media);
}

static struct kobj_attribute settings_pre1   = __ATTR_RO(pre1);
static struct kobj_attribute settings_pre2   = __ATTR_RO(pre2);
static struct kobj_attribute settings_pre3   = __ATTR_RO(pre3);
static struct kobj_attribute settings_cursor = __ATTR_RO(cursor);
static struct kobj_attribute settings_post1  = __ATTR_RO(post1);
static struct kobj_attribute settings_post2  = __ATTR_RO(post2);
static struct kobj_attribute settings_media  = __ATTR_RO(media);

static ssize_t osr_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	u8                               osr;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_osr_get(core_lgrp, lane_kobj->asic_lane_num, &osr);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"settings osr show (osr = %u %s)", osr, sl_core_serdes_lane_osr_str(osr));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_core_serdes_lane_osr_str(osr));
}

static ssize_t encoding_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	u8                               encoding;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_encoding_get(core_lgrp, lane_kobj->asic_lane_num, &encoding);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"settings encoding show (encoding = %u %s)",
		encoding, sl_core_serdes_lane_encoding_str(encoding));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_core_serdes_lane_encoding_str(encoding));
}

static ssize_t clocking_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_link             *core_link;
	u16                              clocking;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	sl_core_link_clocking_get(core_link, &clocking);

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		"settings clocking show (clocking = %u %s)",
		clocking, sl_core_serdes_lane_clocking_str(clocking));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_core_serdes_lane_clocking_str(clocking));
}

static ssize_t width_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	u8                               width;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_width_get(core_lgrp, lane_kobj->asic_lane_num, &width);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"settings width show (width = %u %s)",
		width, sl_core_serdes_lane_width_str(width));

	return scnprintf(buf, PAGE_SIZE, "%s\n", sl_core_serdes_lane_width_str(width));
}

static ssize_t dfe_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	u8                               dfe;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_dfe_get(core_lgrp, lane_kobj->asic_lane_num, &dfe);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"settings dfe show (dfe = %s)", (dfe) ? "enabled" : "disabled");

	return scnprintf(buf, PAGE_SIZE, "%s\n", (dfe) ? "enabled" : "disabled");
}

static ssize_t scramble_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;
	struct sl_core_link             *core_link;
	u8                               scramble;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num);

	sl_core_lgrp_scramble_get(core_lgrp, lane_kobj->asic_lane_num, &scramble);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"settings scramble show (scramble = %s)", (scramble) ? "enabled" : "disabled");

	return scnprintf(buf, PAGE_SIZE, "%s\n", (scramble) ? "enabled" : "disabled");
}

static struct kobj_attribute settings_osr      = __ATTR_RO(osr);
static struct kobj_attribute settings_encoding = __ATTR_RO(encoding);
static struct kobj_attribute settings_clocking = __ATTR_RO(clocking);
static struct kobj_attribute settings_width    = __ATTR_RO(width);
static struct kobj_attribute settings_dfe      = __ATTR_RO(dfe);
static struct kobj_attribute settings_scramble = __ATTR_RO(scramble);

static ssize_t link_training_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_link             *core_link;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctrl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no-lane\n");

	core_link = sl_core_link_get(lane_kobj->ctrl_lgrp->ctrl_ldev->num, lane_kobj->ctrl_lgrp->num,
		lane_num_to_link_num(lane_kobj->ctrl_lgrp, lane_kobj->asic_lane_num));
	if (!core_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	if (is_flag_set(core_link->core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN))
		return scnprintf(buf, PAGE_SIZE, "enabled\n");
	else
		return scnprintf(buf, PAGE_SIZE, "disabled\n");
}

static struct kobj_attribute settings_link_training = __ATTR_RO(link_training);

static struct attribute *serdes_lane_settings_attrs[] = {
	/* media */
	&settings_pre1.attr,
	&settings_pre2.attr,
	&settings_pre3.attr,
	&settings_cursor.attr,
	&settings_post1.attr,
	&settings_post2.attr,
	&settings_media.attr,
	/* core */
	&settings_osr.attr,
	&settings_encoding.attr,
	&settings_clocking.attr,
	&settings_width.attr,
	&settings_dfe.attr,
	&settings_scramble.attr,
	&settings_link_training.attr,
	NULL
};
ATTRIBUTE_GROUPS(serdes_lane_settings);

static struct kobj_type serdes_lane_settings_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = serdes_lane_settings_groups,
};

int sl_sysfs_serdes_lane_settings_create(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	int rtn;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane settings create (lgrp = 0x%p)", ctrl_lgrp);

	rtn = kobject_init_and_add(&(ctrl_lgrp->serdes_lane_settings_kobjs[asic_lane_num].kobj),
		&serdes_lane_settings_info, &(ctrl_lgrp->serdes_lane_kobjs[asic_lane_num]), "settings");
	if (rtn) {
		sl_log_err(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
			"serdes lane settings create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&(ctrl_lgrp->serdes_lane_settings_kobjs[asic_lane_num].kobj));
		return -ENOMEM;
	}
	ctrl_lgrp->serdes_lane_settings_kobjs[asic_lane_num].ctrl_lgrp      = ctrl_lgrp;
	ctrl_lgrp->serdes_lane_settings_kobjs[asic_lane_num].asic_lane_num = asic_lane_num;

	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane settings create (serdes_kobj = 0x%p)", &(ctrl_lgrp->serdes_kobj));

	return 0;
}

void sl_sysfs_serdes_lane_settings_delete(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num)
{
	sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane settings delete (lgrp = 0x%p)", ctrl_lgrp);

	kobject_put(&(ctrl_lgrp->serdes_lane_settings_kobjs[asic_lane_num].kobj));
	ctrl_lgrp->serdes_lane_settings_kobjs[asic_lane_num].ctrl_lgrp = NULL;
}
