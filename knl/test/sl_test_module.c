// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include "sl_test.h"
#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"

#define tostr(x)             #x
#define version_str(a, b, c) tostr(a.b.c)
#define SL_TEST_VERSION_STR \
	version_str(SL_TEST_VERSION_MAJOR, SL_TEST_VERSION_MINOR, SL_TEST_VERSION_INC)
#define hash_str(h)         tostr(h)
#define SL_TEST_GIT_HASH_STR hash_str(GIT_HASH)

void sl_test_version_get(int *major, int *minor, int *inc)
{
	if (major)
		*major = SL_TEST_VERSION_MAJOR;
	if (minor)
		*minor = SL_TEST_VERSION_MINOR;
	if (inc)
		*inc = SL_TEST_VERSION_INC;
}
EXPORT_SYMBOL(sl_test_version_get);

char *sl_test_git_hash_str_get(void)
{
	return SL_TEST_GIT_HASH_STR;
}
EXPORT_SYMBOL(sl_test_git_hash_str_get);

static char *sl_test_framework_str(void)
{
#ifdef BUILDSYS_FRAMEWORK_ROSETTA
	return "rosetta";
#else
#ifdef BUILDSYS_FRAMEWORK_CASSINI
	return "cassini";
#else
	return "unknown";
#endif
#endif
}

static int __init sl_test_init(void)
{
	int rtn;

	pr_info("%s: init v" SL_TEST_VERSION_STR " (%s)\n",
		module_name(THIS_MODULE), sl_test_framework_str());
	pr_info("%s: git hash " SL_TEST_GIT_HASH_STR "\n", module_name(THIS_MODULE));

	rtn = sl_test_debugfs_create();
	if (rtn != 0) {
		pr_err("%s: sl_test_debugfs_create failed [%d]\n",
			module_name(THIS_MODULE), rtn);
		return rtn;
	}

	return 0;
}
module_init(sl_test_init);

static void __exit sl_test_exit(void)
{
	pr_info("%s: exit\n", module_name(THIS_MODULE));

	sl_test_debugfs_destroy();

	sl_test_ldev_exit();
}
module_exit(sl_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slingshot Platform Team <hpcdev_ss_plat@hpe.com>");
MODULE_DESCRIPTION("Slingshot link test module");
MODULE_VERSION(SL_TEST_VERSION_STR);
