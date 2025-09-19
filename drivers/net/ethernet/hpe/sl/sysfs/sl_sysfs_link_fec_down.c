// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs_link_fec.h"
#include "sl_ctrl_link.h"
#include "sl_core_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_link_priv.h"
#include "sl_core_link_fec.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ccw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link              *ctrl_link;
	struct sl_core_link_fec_cw_cntrs  cw_cntrs;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.down_kobj);

	sl_ctrl_link_fec_down_cache_cw_cntrs_get(ctrl_link, &cw_cntrs);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"ccw show (link = 0x%p, ccw = %llu)", ctrl_link, cw_cntrs.ccw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", cw_cntrs.ccw);
}

static ssize_t ucw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link              *ctrl_link;
	struct sl_core_link_fec_cw_cntrs  cw_cntrs;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.down_kobj);

	sl_ctrl_link_fec_down_cache_cw_cntrs_get(ctrl_link, &cw_cntrs);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"ucw show (link = 0x%p, ucw = %llu)", ctrl_link, cw_cntrs.ucw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", cw_cntrs.ucw);
}

static ssize_t gcw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_link              *ctrl_link;
	struct sl_core_link_fec_cw_cntrs  cw_cntrs;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.down_kobj);

	sl_ctrl_link_fec_down_cache_cw_cntrs_get(ctrl_link, &cw_cntrs);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"gcw show (link = 0x%p, gcw = %llu)", ctrl_link, cw_cntrs.gcw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", cw_cntrs.gcw);
}

static struct kobj_attribute link_fec_ccw = __ATTR_RO(ccw);
static struct kobj_attribute link_fec_ucw = __ATTR_RO(ucw);
static struct kobj_attribute link_fec_gcw = __ATTR_RO(gcw);

static struct attribute *link_fec_down_attrs[] = {
	&link_fec_ccw.attr,
	&link_fec_ucw.attr,
	&link_fec_gcw.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_down);

static struct kobj_type link_fec_down = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_down_groups,
};

static struct attribute *link_fec_down_lane_attrs[] = {
	NULL
};
ATTRIBUTE_GROUPS(link_fec_down_lane);

static struct kobj_type link_fec_down_lane = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_down_lane_groups,
};

static ssize_t link_fec_down_fecl_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	struct sl_ctrl_link_fecl_kobj       *fecl_kobj;
	struct sl_core_link_fec_lane_cntrs  lane_cntrs;
	u8                                  fecl_num;

	fecl_kobj = container_of(kobj, struct sl_ctrl_link_fecl_kobj, kobj);
	if (!fecl_kobj->ctrl_link)
		return scnprintf(buf, PAGE_SIZE, "no-link\n");

	sl_ctrl_link_fec_down_cache_lane_cntrs_get(fecl_kobj->ctrl_link, &lane_cntrs);

	fecl_num = ((4 * fecl_kobj->lane_num) + num);

	sl_log_dbg(fecl_kobj->ctrl_link, LOG_BLOCK, LOG_NAME,
		"current fecl show (link = 0x%p, num = %u, lane_num = %u, fecl %u = %llu)",
		fecl_kobj->ctrl_link, num, fecl_kobj->lane_num, fecl_num, lane_cntrs.lanes[fecl_num]);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", lane_cntrs.lanes[fecl_num]);
}

#define link_fec_down_fecl(_num)                                                                               \
	static inline ssize_t fecl##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                      \
		return link_fec_down_fecl_show(kobj, kattr, buf, (_num));                                      \
	}                                                                                                      \
	static struct kobj_attribute link_fecl##_num = __ATTR_RO(fecl##_num)

static ssize_t link_fec_down_bin_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	struct sl_ctrl_link                 *ctrl_link;
	struct sl_core_link_fec_tail_cntrs  tail_cntrs;

	ctrl_link = container_of(kobj, struct sl_ctrl_link, fec.down_tail_kobj);

	sl_ctrl_link_fec_down_cache_tail_cntrs_get(ctrl_link, &tail_cntrs);

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"down bin show (link = 0x%p, bin %u = %llu)",
		ctrl_link, num, tail_cntrs.ccw_bins[num]);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", tail_cntrs.ccw_bins[num]);
}

#define link_fec_down_tail0(_num)                                                                       \
	static ssize_t bin0##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                               \
		return link_fec_down_bin_show(kobj, kattr, buf, _num);                                  \
	}                                                                                               \
	static struct kobj_attribute fec_bin0##_num = __ATTR_RO(bin0##_num)

#define link_fec_down_tail(_num)                                                                              \
	static inline ssize_t bin##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                     \
		return link_fec_down_bin_show(kobj, kattr, buf, (_num));                                      \
	}                                                                                                     \
	static struct kobj_attribute fec_bin##_num = __ATTR_RO(bin##_num)

link_fec_down_fecl(0);
link_fec_down_fecl(1);
link_fec_down_fecl(2);
link_fec_down_fecl(3);

static struct attribute *link_fec_down_fecl_attrs[] = {
	&link_fecl0.attr,
	&link_fecl1.attr,
	&link_fecl2.attr,
	&link_fecl3.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_down_fecl);

static struct kobj_type link_fec_down_fecl = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_down_fecl_groups,
};

link_fec_down_tail0(0);
link_fec_down_tail0(1);
link_fec_down_tail0(2);
link_fec_down_tail0(3);
link_fec_down_tail0(4);
link_fec_down_tail0(5);
link_fec_down_tail0(6);
link_fec_down_tail0(7);
link_fec_down_tail0(8);
link_fec_down_tail0(9);
link_fec_down_tail(10);
link_fec_down_tail(11);
link_fec_down_tail(12);
link_fec_down_tail(13);
link_fec_down_tail(14);

static struct attribute *link_fec_down_tail_attrs[] = {
	&fec_bin00.attr,
	&fec_bin01.attr,
	&fec_bin02.attr,
	&fec_bin03.attr,
	&fec_bin04.attr,
	&fec_bin05.attr,
	&fec_bin06.attr,
	&fec_bin07.attr,
	&fec_bin08.attr,
	&fec_bin09.attr,
	&fec_bin10.attr,
	&fec_bin11.attr,
	&fec_bin12.attr,
	&fec_bin13.attr,
	&fec_bin14.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_down_tail);

static struct kobj_type link_fec_down_tail = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_down_tail_groups,
};

int sl_sysfs_link_fec_down_create(struct sl_ctrl_link *ctrl_link)
{
	int rtn;
	int x;
	int out;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"link fec down create (num = %u)", ctrl_link->num);

	rtn = kobject_init_and_add(&ctrl_link->fec.down_kobj, &link_fec_down,
		&ctrl_link->fec.kobj, "down");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link fec down create kobject_init_and_add failed [%d]", rtn);
		goto out_down;
	}

	rtn = kobject_init_and_add(&ctrl_link->fec.down_lane_kobj,
		&link_fec_down_lane, &ctrl_link->fec.down_kobj, "lane");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link fec down lane create kobject_init_and_add failed [%d]", rtn);
		goto out_down_lane;
	}
	for (x = 0; x < SL_MAX_LANES; ++x) {
		rtn = kobject_init_and_add(&ctrl_link->fec.down_fecl_kobjs[x].kobj,
			&link_fec_down_fecl, &ctrl_link->fec.down_lane_kobj, "%d", x);
		if (rtn) {
			sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
				"link fec down fecl create kobject_init_and_add failed [%d]", rtn);
			goto out_down_fecl;
		}
		ctrl_link->fec.down_fecl_kobjs[x].ctrl_link = ctrl_link;
		ctrl_link->fec.down_fecl_kobjs[x].lane_num = x;
	}

	rtn = kobject_init_and_add(&ctrl_link->fec.down_tail_kobj,
		&link_fec_down_tail, &ctrl_link->fec.down_kobj, "tail");
	if (rtn) {
		sl_log_err(ctrl_link, LOG_BLOCK, LOG_NAME,
			"link fec down tail create kobject_init_and_add failed [%d]", rtn);
		goto out_down_tail;
	}

	return 0;

out_down_tail:
	kobject_put(&ctrl_link->fec.down_tail_kobj);
out_down_fecl:
	kobject_put(&ctrl_link->fec.down_fecl_kobjs[x].kobj);
	for (out = 0; out < x; ++out)
		kobject_put(&ctrl_link->fec.down_fecl_kobjs[out].kobj);
out_down_lane:
	kobject_put(&ctrl_link->fec.down_lane_kobj);
out_down:
	kobject_put(&ctrl_link->fec.down_kobj);

	return rtn;
}

void sl_sysfs_link_fec_down_delete(struct sl_ctrl_link *ctrl_link)
{
	int x;

	sl_log_dbg(ctrl_link, LOG_BLOCK, LOG_NAME,
		"link fec down delete (link_num = %u)", ctrl_link->num);

	kobject_put(&ctrl_link->fec.down_tail_kobj);
	for (x = 0; x < SL_MAX_LANES; ++x)
		kobject_put(&ctrl_link->fec.down_fecl_kobjs[x].kobj);
	kobject_put(&ctrl_link->fec.down_lane_kobj);
	kobject_put(&ctrl_link->fec.down_kobj);
}
