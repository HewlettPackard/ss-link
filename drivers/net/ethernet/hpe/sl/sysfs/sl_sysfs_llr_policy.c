// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "sl_sysfs.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_llr.h"
#include "sl_core_llr.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t continuous_tries_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	struct sl_ctrl_llr   *ctrl_llr;
	struct sl_llr_policy  llr_policy;

	ctrl_llr = container_of(kobj, struct sl_ctrl_llr, policy_kobj);
	sl_core_llr_policy_get(ctrl_llr->ctrl_lgrp->ctrl_ldev->num,
			       ctrl_llr->ctrl_lgrp->num, ctrl_llr->num, &llr_policy);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		(llr_policy.options & SL_LLR_POLICY_OPT_CONTINUOUS_START_TRIES) ? "enabled" : "disabled");
}

static struct kobj_attribute continuous_tries = __ATTR_RO(continuous_tries);

static struct attribute *llr_policy_attrs[] = {
	&continuous_tries.attr,
	NULL
};
ATTRIBUTE_GROUPS(llr_policy);

static struct kobj_type llr_policy = {
	.sysfs_ops      = &kobj_sysfs_ops,
	.default_groups = llr_policy_groups,
};

int sl_sysfs_llr_policy_create(struct sl_ctrl_llr *ctrl_llr)
{
	int rtn;

	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr policy create (num = %u)", ctrl_llr->num);

	rtn = kobject_init_and_add(&ctrl_llr->policy_kobj, &llr_policy, &ctrl_llr->kobj, "policies");
	if (rtn) {
		sl_log_err(ctrl_llr, LOG_BLOCK, LOG_NAME,
			"llr policy create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&ctrl_llr->policy_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_llr_policy_delete(struct sl_ctrl_llr *ctrl_llr)
{
	sl_log_dbg(ctrl_llr, LOG_BLOCK, LOG_NAME, "llr policy delete (num = %u)", ctrl_llr->num);

	kobject_put(&ctrl_llr->policy_kobj);
}
