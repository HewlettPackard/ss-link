// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_core_link.h"

#include "sl_sysfs_link_fec_current.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t ccw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	u64                  ccw;
	struct sl_core_link *core_link;

	core_link = container_of(kobj, struct sl_core_link, fec.current_kobj);

	rtn = sl_core_link_fec_ccw_get(core_link, &ccw);
	if (rtn == -ENOLINK) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "ccw show sl_core_link_fec_ccw_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "no-link\n");
	}
	if (rtn) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "ccw show sl_core_link_fec_ccw_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "io-error\n");
	}

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "ccw show (ccw = %llu)", ccw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", ccw);
}

static ssize_t ucw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                  rtn;
	u64                  ucw;
	struct sl_core_link *core_link;

	core_link = container_of(kobj, struct sl_core_link, fec.current_kobj);

	rtn = sl_core_link_fec_ucw_get(core_link, &ucw);
	if (rtn == -ENOLINK) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "ucw show sl_core_link_fec_ucw_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "no-link\n");
	}
	if (rtn) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "ucw show sl_core_link_fec_ucw_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "io-error\n");
	}

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "ucw show (ucw = %llu)", ucw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", ucw);
}

static ssize_t gcw_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int		     rtn;
	u64                  gcw;
	struct sl_core_link *core_link;

	core_link = container_of(kobj, struct sl_core_link, fec.current_kobj);

	rtn = sl_core_link_fec_gcw_get(core_link, &gcw);
	if (rtn == -ENOLINK) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "gcw show sl_core_link_fec_gcw_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "no-link\n");
	}
	if (rtn) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "gcw show sl_core_link_fec_gcw_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "io-error\n");
	}

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "gcw show (link = 0x%p, gcw = %llu)", core_link, gcw);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", gcw);
}

static struct kobj_attribute link_fec_ccw = __ATTR_RO(ccw);
static struct kobj_attribute link_fec_ucw = __ATTR_RO(ucw);
static struct kobj_attribute link_fec_gcw = __ATTR_RO(gcw);

static struct attribute *link_fec_current_attrs[] = {
	&link_fec_ccw.attr,
	&link_fec_ucw.attr,
	&link_fec_gcw.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_current);

static struct kobj_type link_fec_current = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_current_groups,
};

static struct attribute *link_fec_current_lane_attrs[] = {
	NULL
};
ATTRIBUTE_GROUPS(link_fec_current_lane);

static struct kobj_type link_fec_current_lane = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_current_lane_groups,
};

static ssize_t link_fec_current_fecl_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	int                            rtn;
	struct sl_core_link_fecl_kobj *fecl_kobj;
	struct sl_core_link           *core_link;
	u8                             fecl_num;
	u64                            fecl;

	fecl_kobj = container_of(kobj, struct sl_core_link_fecl_kobj, kobj);

	core_link = fecl_kobj->core_link;

	fecl_num = ((4 * fecl_kobj->lane_num) + num);

	rtn = sl_core_link_fec_lane_cntr_get(core_link, fecl_num, &fecl);
	if (rtn == -ENOLINK) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "current fecl show sl_core_link_fec_lane_cntr_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "no-link\n");
	}
	if (rtn) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "current fecl show sl_core_link_fec_lane_cntr_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "io-error\n");
	}

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "current fecl show (num = %u, lane_num = %u, fecl %u = %llu)",
		   num, fecl_kobj->lane_num, fecl_num, fecl);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", fecl);
}

#define link_fec_current_fecl(_num)                                                                            \
	static inline ssize_t fecl##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                      \
		return link_fec_current_fecl_show(kobj, kattr, buf, (_num));                                   \
	}                                                                                                      \
	static struct kobj_attribute link_fecl##_num = __ATTR_RO(fecl##_num)

static ssize_t link_fec_current_bin_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf, u8 num)
{
	int                  rtn;
	struct sl_core_link *core_link;
	u64                  ccw_bin;

	core_link = container_of(kobj, struct sl_core_link, fec.current_tail_kobj);

	rtn = sl_core_link_fec_tail_cntr_get(core_link, num, &ccw_bin);
	if (rtn == -ENOLINK) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "current bin show sl_core_link_fec_tail_cntr_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "no-link\n");
	}
	if (rtn) {
		sl_log_err_trace(core_link, LOG_BLOCK, LOG_NAME,
				 "current bin show sl_core_link_fec_tail_cntr_get failed [%d]", rtn);
		return scnprintf(buf, PAGE_SIZE, "io-error\n");
	}

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME, "current bin show (bin %u = %llu)", num, ccw_bin);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", ccw_bin);
}

#define link_fec_current_tail0(_num)                                                                    \
	static ssize_t bin0##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                               \
		return link_fec_current_bin_show(kobj, kattr, buf, _num);                               \
	}                                                                                               \
	static struct kobj_attribute fec_bin0##_num = __ATTR_RO(bin0##_num)

#define link_fec_current_tail(_num)                                                                           \
	static inline ssize_t bin##_num##_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf) \
	{                                                                                                     \
		return link_fec_current_bin_show(kobj, kattr, buf, (_num));                                   \
	}                                                                                                     \
	static struct kobj_attribute fec_bin##_num = __ATTR_RO(bin##_num)

link_fec_current_fecl(0);
link_fec_current_fecl(1);
link_fec_current_fecl(2);
link_fec_current_fecl(3);

static struct attribute *link_fec_current_fecl_attrs[] = {
	&link_fecl0.attr,
	&link_fecl1.attr,
	&link_fecl2.attr,
	&link_fecl3.attr,
	NULL
};
ATTRIBUTE_GROUPS(link_fec_current_fecl);

static struct kobj_type link_fec_current_fecl = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_current_fecl_groups,
};

link_fec_current_tail0(0);
link_fec_current_tail0(1);
link_fec_current_tail0(2);
link_fec_current_tail0(3);
link_fec_current_tail0(4);
link_fec_current_tail0(5);
link_fec_current_tail0(6);
link_fec_current_tail0(7);
link_fec_current_tail0(8);
link_fec_current_tail0(9);
link_fec_current_tail(10);
link_fec_current_tail(11);
link_fec_current_tail(12);
link_fec_current_tail(13);
link_fec_current_tail(14);

static struct attribute *link_fec_current_tail_attrs[] = {
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
ATTRIBUTE_GROUPS(link_fec_current_tail);

static struct kobj_type link_fec_current_tail = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = link_fec_current_tail_groups,
};

int sl_sysfs_link_fec_current_create(struct sl_core_link *core_link, struct kobject *parent_kobj)
{
	int rtn;
	int x;
	int out;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "link fec current create (num = %u)", core_link->num);

	rtn = kobject_init_and_add(&core_link->fec.current_kobj, &link_fec_current, parent_kobj, "current");
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME,
			   "link fec current create kobject_init_and_add failed [%d]", rtn);
		goto out_current;
	}

	rtn = kobject_init_and_add(&core_link->fec.current_lane_kobj,
				   &link_fec_current_lane, &core_link->fec.current_kobj, "lane");
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME,
			   "link fec current lane create kobject_init_and_add failed [%d]", rtn);
		goto out_current_lane;
	}
	for (x = 0; x < SL_MAX_LANES; ++x) {
		rtn = kobject_init_and_add(&core_link->fec.current_fecl_kobjs[x].kobj,
					   &link_fec_current_fecl, &core_link->fec.current_lane_kobj, "%d", x);
		if (rtn) {
			sl_log_err(core_link, LOG_BLOCK, LOG_NAME,
				   "link fec current fecl create kobject_init_and_add failed [%d]", rtn);
			goto out_current_fecl;
		}
		core_link->fec.current_fecl_kobjs[x].core_link = core_link;
		core_link->fec.current_fecl_kobjs[x].lane_num = x;
	}

	rtn = kobject_init_and_add(&core_link->fec.current_tail_kobj,
				   &link_fec_current_tail, &core_link->fec.current_kobj, "tail");
	if (rtn) {
		sl_log_err(core_link, LOG_BLOCK, LOG_NAME,
			   "link fec current tail create kobject_init_and_add failed [%d]", rtn);
		goto out_current_tail;
	}

	return 0;

out_current_tail:
	kobject_put(&core_link->fec.current_tail_kobj);
out_current_fecl:
	kobject_put(&core_link->fec.current_fecl_kobjs[x].kobj);
	for (out = 0; out < x; ++out)
		kobject_put(&core_link->fec.current_fecl_kobjs[out].kobj);
out_current_lane:
	kobject_put(&core_link->fec.current_lane_kobj);
out_current:
	kobject_put(&core_link->fec.current_kobj);

	return rtn;
}

void sl_sysfs_link_fec_current_delete(struct sl_core_link *core_link)
{
	int x;

	sl_log_dbg(core_link, LOG_BLOCK, LOG_NAME,
		   "link fec current delete (link_num = %u)", core_link->num);

	kobject_put(&core_link->fec.current_tail_kobj);
	for (x = 0; x < SL_MAX_LANES; ++x)
		kobject_put(&core_link->fec.current_fecl_kobjs[x].kobj);
	kobject_put(&core_link->fec.current_lane_kobj);
	kobject_put(&core_link->fec.current_kobj);
}
