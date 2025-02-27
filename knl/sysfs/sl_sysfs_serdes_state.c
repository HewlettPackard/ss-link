// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_link.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_str.h"
#include "sl_sysfs_serdes_state.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t tx_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no_lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctl_lgrp->ctl_ldev->num, lane_kobj->ctl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"state tx show (asic_lane_num = %u, state = %u %s)",
		lane_kobj->asic_lane_num, core_lgrp->serdes.lane_state[lane_kobj->asic_lane_num].tx,
		sl_core_serdes_lane_state_str(core_lgrp->serdes.lane_state[lane_kobj->asic_lane_num].tx));

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		sl_core_serdes_lane_state_str(core_lgrp->serdes.lane_state[lane_kobj->asic_lane_num].tx));
}

static ssize_t rx_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_lgrp_serdes_lane_kobj *lane_kobj;
	struct sl_core_lgrp             *core_lgrp;

	lane_kobj = container_of(kobj, struct sl_lgrp_serdes_lane_kobj, kobj);
	if (!lane_kobj->ctl_lgrp)
		return scnprintf(buf, PAGE_SIZE, "no_lane\n");

	core_lgrp = sl_core_lgrp_get(lane_kobj->ctl_lgrp->ctl_ldev->num, lane_kobj->ctl_lgrp->num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"state rx show (asic_lane_num = %u, state = %u %s)",
		lane_kobj->asic_lane_num, core_lgrp->serdes.lane_state[lane_kobj->asic_lane_num].rx,
		sl_core_serdes_lane_state_str(core_lgrp->serdes.lane_state[lane_kobj->asic_lane_num].rx));

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		sl_core_serdes_lane_state_str(core_lgrp->serdes.lane_state[lane_kobj->asic_lane_num].rx));
}

static struct kobj_attribute state_tx = __ATTR_RO(tx);
static struct kobj_attribute state_rx = __ATTR_RO(rx);

static struct attribute *serdes_lane_state_attrs[] = {
	&state_tx.attr,
	&state_rx.attr,
	NULL,
};
ATTRIBUTE_GROUPS(serdes_lane_state);

static struct kobj_type serdes_lane_state_info = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = serdes_lane_state_groups,
};

int sl_sysfs_serdes_lane_state_create(struct sl_ctl_lgrp *ctl_lgrp, u8 asic_lane_num)
{
	int rtn;

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane state create (lgrp = 0x%p)", ctl_lgrp);

	rtn = kobject_init_and_add(&(ctl_lgrp->serdes_lane_state_kobjs[asic_lane_num].kobj),
		&serdes_lane_state_info, &(ctl_lgrp->serdes_lane_kobjs[asic_lane_num]), "state");
	if (rtn) {
		sl_log_err(ctl_lgrp, LOG_BLOCK, LOG_NAME,
			"serdes lane state create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&(ctl_lgrp->serdes_lane_state_kobjs[asic_lane_num].kobj));
		return -ENOMEM;
	}
	ctl_lgrp->serdes_lane_state_kobjs[asic_lane_num].ctl_lgrp      = ctl_lgrp;
	ctl_lgrp->serdes_lane_state_kobjs[asic_lane_num].asic_lane_num = asic_lane_num;

	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane state create (serdes_kobj = 0x%p)", &(ctl_lgrp->serdes_kobj));
	return 0;
}

void sl_sysfs_serdes_lane_state_delete(struct sl_ctl_lgrp *ctl_lgrp, u8 asic_lane_num)
{
	sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME,
		"serdes lane state delete (lgrp = 0x%p)", ctl_lgrp);

	kobject_put(&(ctl_lgrp->serdes_lane_state_kobjs[asic_lane_num].kobj));
	ctl_lgrp->serdes_lane_state_kobjs[asic_lane_num].ctl_lgrp = NULL;
}
