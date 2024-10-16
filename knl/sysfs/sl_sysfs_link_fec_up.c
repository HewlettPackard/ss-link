// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs_fec.h"
#include "sl_ctl_link.h"
#include "sl_core_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_link_priv.h"
#include "sl_core_link_fec.h"
#include "sl_test_common.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ccw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link               *ctl_link;
	struct sl_core_link_fec_cw_cntrs  cw_cntrs;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_kobj);

	sl_ctl_link_fec_up_cache_cw_cntrs_get(ctl_link, &cw_cntrs);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"ccw show (link = 0x%p, ccw = %llu)", ctl_link, cw_cntrs.ccw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", cw_cntrs.ccw);
}

static ssize_t ucw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link               *ctl_link;
	struct sl_core_link_fec_cw_cntrs  cw_cntrs;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_kobj);

	sl_ctl_link_fec_up_cache_cw_cntrs_get(ctl_link, &cw_cntrs);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"ucw show (link = 0x%p, ucw = %llu)", ctl_link, cw_cntrs.ucw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", cw_cntrs.ucw);
}

static ssize_t gcw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_link               *ctl_link;
	struct sl_core_link_fec_cw_cntrs  cw_cntrs;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_kobj);

	sl_ctl_link_fec_up_cache_cw_cntrs_get(ctl_link, &cw_cntrs);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"gcw show (link = 0x%p, gcw = %llu)", ctl_link, cw_cntrs.gcw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", cw_cntrs.gcw);
}

static struct kobj_attribute link_fec_ccw = __ATTR_RO(ccw);
static struct kobj_attribute link_fec_ucw = __ATTR_RO(ucw);
static struct kobj_attribute link_fec_gcw = __ATTR_RO(gcw);

static struct attribute *link_fec_up_attrs[] = {
	&link_fec_ccw.attr,
	&link_fec_ucw.attr,
	&link_fec_gcw.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_up);

static struct kobj_type link_fec_up = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_up_groups,
};

static struct attribute *link_fec_up_lane_attrs[] = {
	NULL
};
ATTRIBUTE_GROUPS(link_fec_up_lane);

static struct kobj_type link_fec_up_lane = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_up_lane_groups,
};

static ssize_t link_fec_up_fecl_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	struct sl_ctl_link_fecl_kobj       *fecl_kobj;
	struct sl_core_link_fec_lane_cntrs  lane_cntrs;
	u8                                  fecl_num;
	struct sl_ctl_link                 *ctl_link;

	fecl_kobj = container_of(kobj, struct sl_ctl_link_fecl_kobj, kobj);
	if (!fecl_kobj->ctl_link)
		return scnprintf(buf, PAGE_SIZE, "no link\n");

	sl_ctl_link_fec_up_cache_lane_cntrs_get(fecl_kobj->ctl_link, &lane_cntrs);

	fecl_num = ((4 * fecl_kobj->lane_num) + num);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"current fecl show (link = 0x%p, num = %u, lane_num = %u, fecl %u = %llu)",
		ctl_link, num, fecl_kobj->lane_num, fecl_num, lane_cntrs.lanes[fecl_num]);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", lane_cntrs.lanes[fecl_num]);
}

#define link_fec_up_fecl(_num)                                                                                 \
	static inline ssize_t fecl##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                      \
		return link_fec_up_fecl_show(kobj, kattr, buf, (_num));                                        \
	}                                                                                                      \
	static struct kobj_attribute link_fecl##_num = __ATTR_RO(fecl##_num)

static ssize_t link_fec_up_bin_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	struct sl_ctl_link                 *ctl_link;
	struct sl_core_link_fec_tail_cntrs  tail_cntrs;

	ctl_link = container_of(kobj, struct sl_ctl_link, fec.up_tail_kobj);

	sl_ctl_link_fec_up_cache_tail_cntrs_get(ctl_link, &tail_cntrs);

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"up bin show (link = 0x%p, bin %u = %llu)",
		ctl_link, num, tail_cntrs.ccw_bins[num]);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", tail_cntrs.ccw_bins[num]);
}

#define link_fec_up_tail0(_num)                                                                         \
	static ssize_t bin0##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                               \
		return link_fec_up_bin_show(kobj, kattr, buf, _num);                                    \
	}                                                                                               \
	static struct kobj_attribute fec_bin0##_num = __ATTR_RO(bin0##_num)

#define link_fec_up_tail(_num)                                                                                \
	static inline ssize_t bin##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                     \
		return link_fec_up_bin_show(kobj, kattr, buf, (_num));                                        \
	}                                                                                                     \
	static struct kobj_attribute fec_bin##_num = __ATTR_RO(bin##_num)

link_fec_up_fecl(0);
link_fec_up_fecl(1);
link_fec_up_fecl(2);
link_fec_up_fecl(3);

static struct attribute *link_fec_up_fecl_attrs[] = {
	&link_fecl0.attr,
	&link_fecl1.attr,
	&link_fecl2.attr,
	&link_fecl3.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_up_fecl);

static struct kobj_type link_fec_up_fecl = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_up_fecl_groups,
};

link_fec_up_tail0(0);
link_fec_up_tail0(1);
link_fec_up_tail0(2);
link_fec_up_tail0(3);
link_fec_up_tail0(4);
link_fec_up_tail0(5);
link_fec_up_tail0(6);
link_fec_up_tail0(7);
link_fec_up_tail0(8);
link_fec_up_tail0(9);
link_fec_up_tail(10);
link_fec_up_tail(11);
link_fec_up_tail(12);
link_fec_up_tail(13);
link_fec_up_tail(14);

static struct attribute *link_fec_up_tail_attrs[] = {
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
ATTRIBUTE_GROUPS(link_fec_up_tail);

static struct kobj_type link_fec_up_tail = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_up_tail_groups,
};

int sl_sysfs_link_fec_up_create(struct sl_ctl_link *ctl_link)
{
	int rtn;
	int x;
	int out;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"link fec up create (num = %u)", ctl_link->num);

	rtn = kobject_init_and_add(&ctl_link->fec.up_kobj, &link_fec_up,
		&ctl_link->fec.kobj, "up");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link fec up create kobject_init_and_add failed [%d]", rtn);
		goto out_up;
	}

	rtn = kobject_init_and_add(&ctl_link->fec.up_lane_kobj,
		&link_fec_up_lane, &ctl_link->fec.up_kobj, "lane");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link fec up lane create kobject_init_and_add failed [%d]", rtn);
		goto out_up_lane;
	}
	for (x = 0; x < SL_MAX_LANES; ++x) {
		rtn = kobject_init_and_add(&ctl_link->fec.up_fecl_kobjs[x].kobj,
			&link_fec_up_fecl, &ctl_link->fec.up_lane_kobj, "%d", x);
		if (rtn) {
			sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
				"link fec up fecl create kobject_init_and_add failed [%d]", rtn);
			goto out_up_fecl;
		}
		ctl_link->fec.up_fecl_kobjs[x].ctl_link = ctl_link;
		ctl_link->fec.up_fecl_kobjs[x].lane_num = x;
	}

	rtn = kobject_init_and_add(&ctl_link->fec.up_tail_kobj,
		&link_fec_up_tail, &ctl_link->fec.up_kobj, "tail");
	if (rtn) {
		sl_log_err(ctl_link, LOG_BLOCK, LOG_NAME,
			"link fec up tail create kobject_init_and_add failed [%d]", rtn);
		goto out_up_tail;
	}

	return 0;

out_up_tail:
	kobject_put(&ctl_link->fec.up_tail_kobj);
out_up_fecl:
	kobject_put(&ctl_link->fec.up_fecl_kobjs[x].kobj);
	for (out = 0; out < x; ++out)
		kobject_put(&ctl_link->fec.up_fecl_kobjs[out].kobj);
out_up_lane:
	kobject_put(&ctl_link->fec.up_lane_kobj);
out_up:
	kobject_put(&ctl_link->fec.up_kobj);

	return rtn;
}

void sl_sysfs_link_fec_up_delete(struct sl_ctl_link *ctl_link)
{
	int x;

	sl_log_dbg(ctl_link, LOG_BLOCK, LOG_NAME,
		"link fec up delete (link_num = %u)", ctl_link->num);

	kobject_put(&ctl_link->fec.up_tail_kobj);
	for (x = 0; x < SL_MAX_LANES; ++x)
		kobject_put(&ctl_link->fec.up_fecl_kobjs[x].kobj);
	kobject_put(&ctl_link->fec.up_lane_kobj);
	kobject_put(&ctl_link->fec.up_kobj);
}
