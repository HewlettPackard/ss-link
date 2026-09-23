// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/errno.h>
#include <linux/kobject.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/types.h>

#include "sl_asic.h"
#include "sl_test_common.h"
#include "log/sl_log.h"
#include "sl_test_pdev.h"

#define LOG_BLOCK "pdev"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

#define PORT_SYSFS_NAME "test_port"

struct sl_test_pdev_port {
	u8             port_num;
	bool           initialized;
	struct kobject kobj;
	struct kref    ref_count;
};

struct sl_test_pdev_pgrp {
	u8                       pgrp_num;
	bool                     test_port_active;
	struct kobject           test_port_kobj;           /* "test_port" sysfs under port number */
	struct sl_test_pdev_port ports[SL_ASIC_MAX_LINKS];
};

struct sl_test_pdev {
	u8                       dev_num;
	bool                     initialized;
	struct sl_test_pdev_pgrp pgrps[SL_ASIC_MAX_LGRPS];
};

static struct sl_test_pdev test_pdevs[SL_ASIC_MAX_LDEVS];
static DEFINE_MUTEX(test_pdev_lock);

static void sl_test_pdev_kobj_release(struct kobject *kobj)
{
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "kobj_release (kobj = 0x%p)", kobj);
}

static struct kobj_type sl_test_pdev_kobj_type = {
	.sysfs_ops = &kobj_sysfs_ops,
	.release = sl_test_pdev_kobj_release,
};

static void sl_test_pdev_port_release(struct kref *ref_count)
{
	struct sl_test_pdev_port *port;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "port_release (ref_count = 0x%p)", ref_count);

	port = container_of(ref_count, struct sl_test_pdev_port, ref_count);
	port->initialized = false;
	kobject_put(&port->kobj);
}

static void sl_test_pdev_pgrp_remove(struct sl_test_pdev_pgrp *pgrp)
{
	int                       port_num;
	struct sl_test_pdev_port *port;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "pgrp_remove (pgrp_num = %u)", pgrp->pgrp_num);

	for (port_num = 0; port_num < SL_ASIC_MAX_LINKS; ++port_num) {
		port = &pgrp->ports[port_num];

		if (!port->initialized)
			continue;

		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
			   "pgrp_remove init port (pgrp_num = %u, port_num = %u)",
			   pgrp->pgrp_num, port->port_num);

		if (!kref_put(&port->ref_count, sl_test_pdev_port_release))
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				   "pgrp_remove init port referenced (pgrp_num = %u, port_num = %u)",
				   pgrp->pgrp_num, port->port_num);
	}

	if (pgrp->test_port_active) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
			   "pgrp_remove test_port (pgrp_num = %u)",
			   pgrp->pgrp_num);

		pgrp->test_port_active = false;
		kobject_del(&pgrp->test_port_kobj);
	}
}

static int sl_test_pdev_init(u8 test_pdev_num)
{
	int                  rtn;
	struct sl_test_pdev *test_pdev;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "init (test_pdev_num = %u)", test_pdev_num);

	if (test_pdev_num >= SL_ASIC_MAX_LDEVS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "init invalid (test_pdev_num = %u)", test_pdev_num);
		return -EINVAL;
	}

	mutex_lock(&test_pdev_lock);
	test_pdev = &test_pdevs[test_pdev_num];

	if (test_pdev->initialized) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "device already initialized");
		rtn = -EALREADY;
		goto out_unlock;
	}

	test_pdev->dev_num = test_pdev_num;
	test_pdev->initialized = true;

	rtn = 0;
out_unlock:
	mutex_unlock(&test_pdev_lock);

	return rtn;
}

int sl_test_pdev_pgrp_add(u8 test_pdev_num, u8 pgrp_num)
{
	int                      rtn;
	int                      port_num;
	struct sl_test_pdev      *test_pdev;
	struct sl_test_pdev_pgrp *pgrp;
	struct sl_test_pdev_port *port;
	struct kobject           *pgrp_kobj;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "pgrp_add (test_pdev_num = %u, pgrp_num = %u)",
		   test_pdev_num, pgrp_num);

	if (test_pdev_num >= SL_ASIC_MAX_LDEVS || pgrp_num >= SL_ASIC_MAX_LGRPS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "pgrp_add invalid (test_pdev_num = %u, pgrp_num = %u)",
			   test_pdev_num, pgrp_num);
		return -EINVAL;
	}

	mutex_lock(&test_pdev_lock);
	test_pdev = &test_pdevs[test_pdev_num];
	pgrp = &test_pdev->pgrps[pgrp_num];

	if (!test_pdev->initialized) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "pgrp_add dev not initialized (test_pdev_num = %u)",
			   test_pdev_num);
		rtn = -ENODEV;
		goto out_unlock;
	}

	if (pgrp->test_port_active) {
		rtn = 0;
		goto out_unlock;
	}

	pgrp->pgrp_num = pgrp_num;
	pgrp_kobj = sl_test_lgrp_kobj_get(test_pdev_num, pgrp_num);
	if (!pgrp_kobj) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "pgrp_add lgrp_kobj_get failed (test_pdev_num = %u, pgrp_num = %u)",
			   test_pdev_num, pgrp_num);
		rtn = -ENODEV;
		goto out_unlock;
	}

	rtn = kobject_init_and_add(&pgrp->test_port_kobj,
				   &sl_test_pdev_kobj_type, pgrp_kobj, PORT_SYSFS_NAME);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "pgrp_add kobject_init_and_add failed (test_pdev_num = %u, pgrp_num = %u) [%d]",
			   test_pdev_num, pgrp_num, rtn);
		kobject_put(&pgrp->test_port_kobj);
		goto out_unlock;
	}
	pgrp->test_port_active = true;

	for (port_num = 0; port_num < SL_ASIC_MAX_LINKS; ++port_num) {
		port = &pgrp->ports[port_num];
		port->port_num = port_num;

		rtn = kobject_init_and_add(&port->kobj, &sl_test_pdev_kobj_type,
					   &pgrp->test_port_kobj, "%u", port_num);
		if (rtn) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				   "pgrp_add port kobject_init_and_add failed "
				   "(test_pdev_num = %u, pgrp_num = %u, port_num = %u) [%d]",
				   test_pdev_num, pgrp_num, port_num, rtn);
			kobject_put(&port->kobj);
			sl_test_pdev_pgrp_remove(pgrp);
			goto out_unlock;
		}
		kref_init(&port->ref_count);
		port->initialized = true;
	}

out_unlock:
	mutex_unlock(&test_pdev_lock);

	return rtn;
}

static void sl_test_pdev_exit(u8 test_pdev_num)
{
	int                  pgrp_num;
	struct sl_test_pdev *test_pdev;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "exit (test_pdev_num = %u)", test_pdev_num);

	mutex_lock(&test_pdev_lock);
	test_pdev = &test_pdevs[test_pdev_num];

	if (!test_pdev->initialized)
		goto out_unlock;

	test_pdev->initialized = false;
	for (pgrp_num = 0; pgrp_num < SL_ASIC_MAX_LGRPS; ++pgrp_num)
		sl_test_pdev_pgrp_remove(&test_pdev->pgrps[pgrp_num]);

out_unlock:
	mutex_unlock(&test_pdev_lock);
}

struct kobject *sl_test_pdev_port_kobj_get(u8 test_pdev_num, u8 pgrp_num, u8 port_num)
{
	struct sl_test_pdev      *test_pdev;
	struct sl_test_pdev_port *port;
	struct kobject           *port_kobj = NULL;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "port_kobj_get (test_pdev_num = %u, pgrp_num = %u, port_num = %u)",
		   test_pdev_num, pgrp_num, port_num);

	if (test_pdev_num >= SL_ASIC_MAX_LDEVS || pgrp_num >= SL_ASIC_MAX_LGRPS ||
	    port_num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "port_kobj_get invalid (test_pdev_num = %u, pgrp_num = %u, port_num = %u)",
			   test_pdev_num, pgrp_num, port_num);
		return NULL;
	}

	mutex_lock(&test_pdev_lock);
	test_pdev = &test_pdevs[test_pdev_num];

	if (!test_pdev->initialized) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "port_kobj_get not initialized (test_pdev_num = %u)",
			   test_pdev_num);
		goto out_unlock;
	}

	port = &test_pdev->pgrps[pgrp_num].ports[port_num];

	if (!port->initialized) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "port_kobj_get not initialized (test_pdev_num = %u, pgrp_num = %u, port_num = %u)",
			   test_pdev_num, pgrp_num, port_num);
		goto out_unlock;
	}

	port_kobj = &port->kobj;

	if (!kref_get_unless_zero(&port->ref_count)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "port_kobj_get no reference (test_pdev_num = %u, pgrp_num = %u, port_num = %u)",
			   test_pdev_num, pgrp_num, port_num);
		port_kobj = NULL;
	}

out_unlock:
	mutex_unlock(&test_pdev_lock);

	return port_kobj;
}

int sl_test_pdev_port_kobj_put(u8 test_pdev_num, u8 pgrp_num, u8 port_num)
{
	struct sl_test_pdev      *test_pdev;
	struct sl_test_pdev_port *port;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "port_kobj_put (test_pdev_num = %u, pgrp_num = %u, port_num = %u)",
		   test_pdev_num, pgrp_num, port_num);

	if (test_pdev_num >= SL_ASIC_MAX_LDEVS || pgrp_num >= SL_ASIC_MAX_LGRPS ||
	    port_num >= SL_ASIC_MAX_LINKS) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "port_kobj_put invalid (test_pdev_num = %u, pgrp_num = %u, port_num = %u)",
			   test_pdev_num, pgrp_num, port_num);
		return -EINVAL;
	}

	mutex_lock(&test_pdev_lock);
	test_pdev = &test_pdevs[test_pdev_num];

	port = &test_pdev->pgrps[pgrp_num].ports[port_num];
	if (!port->initialized) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "port_kobj_put not initialized (test_pdev_num = %u, pgrp_num = %u, port_num = %u)",
			   test_pdev_num, pgrp_num, port_num);
		mutex_unlock(&test_pdev_lock);
		return -ENOENT;
	}
	kref_put(&port->ref_count, sl_test_pdev_port_release);
	mutex_unlock(&test_pdev_lock);

	return 0;
}

int sl_test_pdev_create(void)
{
	int rtn;
	u8  pdev_num;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "create");

	for (pdev_num = 0; pdev_num < SL_ASIC_MAX_LDEVS; ++pdev_num) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "create (pdev_num = %u)", pdev_num);

		rtn = sl_test_pdev_init(pdev_num);
		if (rtn) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				   "sl_test_pdev_init failed (pdev_num = %u) [%d]",
				   pdev_num, rtn);

			goto out_dev_init;
		}
	}

	return 0;

out_dev_init:
	for (pdev_num = 0; pdev_num < SL_ASIC_MAX_LDEVS; ++pdev_num)
		sl_test_pdev_exit(pdev_num);

	return rtn;
}

void sl_test_pdev_remove(void)
{
	u8 pdev_num;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "remove");

	for (pdev_num = 0; pdev_num < SL_ASIC_MAX_LDEVS; ++pdev_num) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "remove (pdev_num = %u)", pdev_num);
		sl_test_pdev_exit(pdev_num);
	}
}
