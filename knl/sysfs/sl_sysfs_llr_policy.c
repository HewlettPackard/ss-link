// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"
#include "sl_ctl_llr.h"
#include "sl_core_llr.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t infinite_tries_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctl_llr    *ctl_llr;
	struct sl_llr_policy  llr_policy;

	ctl_llr = container_of(kobj, struct sl_ctl_llr, policy_kobj);
	sl_core_llr_policy_get(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num, &llr_policy);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		(llr_policy.options & SL_LLR_POLICY_OPT_INFINITE_START_TRIES) ? "enabled" : "disabled");
}

static struct kobj_attribute infinite_tries = __ATTR_RO(infinite_tries);

static struct attribute *llr_policy_attrs[] = {
	&infinite_tries.attr,
	NULL
};
ATTRIBUTE_GROUPS(llr_policy);

static struct kobj_type llr_policy = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = llr_policy_groups,
};

int sl_sysfs_llr_policy_create(struct sl_ctl_llr *ctl_llr)
{
	int rtn;

	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "llr policy create (num = %u)", ctl_llr->num);

	rtn = kobject_init_and_add(&ctl_llr->policy_kobj, &llr_policy, &ctl_llr->kobj, "policies");
	if (rtn) {
		sl_log_err(ctl_llr, LOG_BLOCK, LOG_NAME,
			"llr policy create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctl_llr->policy_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_llr_policy_delete(struct sl_ctl_llr *ctl_llr)
{
	sl_log_dbg(ctl_llr, LOG_BLOCK, LOG_NAME, "llr policy delete (num = %u)", ctl_llr->num);

	kobject_put(&ctl_llr->policy_kobj);
}
