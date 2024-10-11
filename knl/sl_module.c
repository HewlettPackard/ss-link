// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/version.h>

#include "sl_kconfig.h"
#include "sl_module.h"

#include "sl.h"
#include "sl_ldev.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "sl_llr.h"
#include "sl_mac.h"

#define SL_DEVICE_NAME "sl"

#define tostr(x)             #x
#define version_str(a, b, c) tostr(a.b.c)
#define SL_VERSION_STR       version_str(SL_VERSION_MAJOR, SL_VERSION_MINOR, SL_VERSION_INC)
#define hash_str(h)          tostr(h)
#define SL_GIT_HASH_STR      hash_str(GIT_HASH)

void sl_version_get(int *major, int *minor, int *inc)
{
	if (major)
		*major = SL_VERSION_MAJOR;
	if (minor)
		*minor = SL_VERSION_MINOR;
	if (inc)
		*inc = SL_VERSION_INC;
}
EXPORT_SYMBOL(sl_version_get);

char *sl_version_str_get(void)
{
	return SL_VERSION_STR;
}

char *sl_git_hash_str_get(void)
{
	return SL_GIT_HASH_STR;
}
EXPORT_SYMBOL(sl_git_hash_str_get);

static char *sl_framework_str(void)
{
#ifdef BUILDSYS_FRAMEWORK_ROSETTA
	return "rosetta";
#else
#ifdef BUILDSYS_FRAMEWORK_CASSINI
	return "cassini";
#else
	return "unknown";
#endif /* BUILDSYS_FRAMEWORK_CASSINI */
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */
}

static dev_t          sl_dev;
static struct class  *sl_class;
static struct device *sl_device;

static int __init sl_init(void)
{
	int rtn;

	pr_info("%s: init v" SL_VERSION_STR " (%s)\n",
		module_name(THIS_MODULE), sl_framework_str());
	pr_info("%s: git hash " SL_GIT_HASH_STR "\n", module_name(THIS_MODULE));

	rtn = alloc_chrdev_region(&sl_dev, 0, 1, SL_DEVICE_NAME);
	if (rtn) {
		pr_err("alloc_chrdev_region failed [%d]", rtn);
		return -EIO;
	}

	#if (defined(RHEL_MAJOR) && (RHEL_MAJOR >= 9) && defined(RHEL_MINOR) && (RHEL_MINOR >= 4)) || (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
		sl_class = class_create(SL_DEVICE_NAME);
	#else
		sl_class = class_create(THIS_MODULE, SL_DEVICE_NAME);
	#endif
	if (IS_ERR(sl_class)) {
		rtn = PTR_ERR(sl_class);
		pr_err("class_create \"%s\" failed [%d]", SL_DEVICE_NAME, rtn);
		goto out_region;
	}

	sl_device = device_create(sl_class, NULL, sl_dev, NULL, SL_DEVICE_NAME);
	if (IS_ERR(sl_device)) {
		rtn = PTR_ERR(sl_device);
		pr_err("device_create failed [%d]", rtn);
		goto out_class;
	}

	sl_ldev_init();
	sl_lgrp_init();
	sl_link_init();
	sl_llr_init();
	sl_mac_init();

	return 0;

out_class:
	class_destroy(sl_class);
out_region:
	unregister_chrdev_region(sl_dev, 1);

	return rtn;
}
module_init(sl_init);

struct device *sl_device_get(void)
{
	return sl_device;
}

static void __exit sl_exit(void)
{
	pr_info("%s: exit\n", module_name(THIS_MODULE));

	device_destroy(sl_class, sl_dev);
	class_destroy(sl_class);
	unregister_chrdev_region(sl_dev, 1);

	sl_ldev_exit();
}
module_exit(sl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slingshot Platform Team <hpcdev_ss_plat@hpe.com>");
MODULE_DESCRIPTION("Slingshot Link Module");
MODULE_VERSION(SL_VERSION_STR);
