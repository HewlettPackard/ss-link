// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>

#include "sl_log.h"
#include "data/sl_core_data_llr.h"

#include "sl_sysfs.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_SYSFS_LOG_NAME

static ssize_t continuous_tries_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
	int                 rtn;
	struct sl_core_llr *core_llr;
	u32                 options;

	core_llr = container_of(kobj, struct sl_core_llr, policy_kobj);

	rtn = sl_core_data_llr_policy_options_get(core_llr, &options);
	if (rtn)
		return scnprintf(buf, PAGE_SIZE, "error\n");

	return scnprintf(buf, PAGE_SIZE, "%s\n",
		(options & SL_LLR_POLICY_OPT_CONTINUOUS_START_TRIES) ? "enabled" : "disabled");
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

int sl_sysfs_llr_policy_create(struct sl_core_llr *core_llr, struct kobject *parent_kobj)
{
	int rtn;

	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME, "llr policy create (num = %u)", core_llr->num);

	rtn = kobject_init_and_add(&core_llr->policy_kobj, &llr_policy, parent_kobj, "policies");
	if (rtn) {
		sl_log_err(core_llr, LOG_BLOCK, LOG_NAME,
			   "llr policy create kobject_init_and_add failed [%d]", rtn);
		kobject_put(&core_llr->policy_kobj);
		return rtn;
	}

	return 0;
}

void sl_sysfs_llr_policy_delete(struct sl_core_llr *core_llr)
{
	sl_log_dbg(core_llr, LOG_BLOCK, LOG_NAME, "llr policy delete (num = %u)", core_llr->num);

	kobject_put(&core_llr->policy_kobj);
}
