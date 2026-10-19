// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024-2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/timekeeping.h>

#include <linux/hpe/sl/sl_lgrp.h>
#include <linux/hpe/sl/sl_media.h>

#include "sl_asic.h"
#include "sl_common.h"
#include "sl_lgrp.h"
#include "sl_test_pdev.h"
#include "log/sl_log.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_llr.h"
#include "sl_test_debugfs_mac.h"
#include "sl_test_common.h"

#define LOG_BLOCK "lgrp"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

#define SL_TEST_EVENT_STR_SIZE 1024
#define SL_TEST_LGRP_NOTIF_NUM_ENTRIES 64

static struct dentry         *lgrp_dir;
static struct sl_lgrp         lgrp;
static struct sl_lgrp_config  lgrp_config;
static struct sl_lgrp_policy  lgrp_policy;

struct lgrp_notif_event {
	struct sl_lgrp_notif_msg msg;
	time64_t                 timestamp;
};

struct lgrp_notif_interface {
	struct mutex      read_lock;
	spinlock_t        data_lock;
	bool              is_pollout_req;
	wait_queue_head_t wait;
	char              event_str[SL_TEST_EVENT_STR_SIZE];
	DECLARE_KFIFO(lgrp_events, struct lgrp_notif_event, SL_TEST_LGRP_NOTIF_NUM_ENTRIES);
};

static struct lgrp_notif_interface lgrp_notif_interfaces[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS];
static DEFINE_MUTEX(lgrp_notif_reg_lock);
static bool lgrp_notif_registered[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS];

enum lgrp_cmd_index {
	LGRP_NEW_CMD,
	LGRP_DEL_CMD,
	LGRP_CONFIG_WRITE_CMD,
	LGRP_POLICY_WRITE_CMD,
	LGRP_NOTIFS_REG_CMD,
	LGRP_NOTIFS_UNREG_CMD,
	NUM_CMDS,
};

static struct cmd_entry lgrp_cmd_list[]  = {
	[LGRP_NEW_CMD]          = { .cmd = "lgrp_new",          .desc = "create lgrp object",                 .call = sl_test_lgrp_new         },
	[LGRP_DEL_CMD]          = { .cmd = "lgrp_del",          .desc = "delete lgrp object",                 .call = sl_test_lgrp_del         },
	[LGRP_CONFIG_WRITE_CMD] = { .cmd = "lgrp_config_write", .desc = "write out the lgrp config from dir", .call = sl_test_lgrp_config_set  },
	[LGRP_POLICY_WRITE_CMD] = { .cmd = "lgrp_policy_write", .desc = "write out the lgrp policy from dir", .call = sl_test_lgrp_policy_set  },
	[LGRP_NOTIFS_REG_CMD]   = { .cmd = "lgrp_notifs_reg",   .desc = "register for all notifications",     .call = sl_test_lgrp_notif_reg   },
	[LGRP_NOTIFS_UNREG_CMD] = { .cmd = "lgrp_notifs_unreg", .desc = "unregister for all notifications",   .call = sl_test_lgrp_notif_unreg },
};

static void is_pollout_req_set(struct lgrp_notif_interface *interface, bool pollout)
{
	spin_lock(&interface->data_lock);
	interface->is_pollout_req = pollout;
	spin_unlock(&interface->data_lock);
}

static bool is_pollout_req(struct lgrp_notif_interface *interface)
{
	bool pollout;

	spin_lock(&interface->data_lock);
	pollout = interface->is_pollout_req;
	spin_unlock(&interface->data_lock);

	return pollout;
}

static void sl_test_lgrp_init(struct sl_lgrp *lgrp, u8 ldev_num, u8 lgrp_num)
{
	lgrp->magic    = SL_LGRP_MAGIC;
	lgrp->ver      = SL_LGRP_VER;
	lgrp->size     = sizeof(*lgrp);
	lgrp->ldev_num = ldev_num;
	lgrp->num      = lgrp_num;
}

static int sl_test_lgrp_cmds_show(struct seq_file *s, void *unused)
{
	return sl_test_cmds_show(s, lgrp_cmd_list, ARRAY_SIZE(lgrp_cmd_list));
}

static int sl_test_lgrp_cmds_open(struct inode *inode, struct file *file)
{
	return single_open(file, sl_test_lgrp_cmds_show, inode->i_private);
}

static const struct file_operations sl_test_lgrp_cmds_fops = {
	.owner   = THIS_MODULE,
	.open    = sl_test_lgrp_cmds_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int sl_test_lgrp_notif_setup(void)
{
	u8 ldev_num;
	u8 lgrp_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
			INIT_KFIFO(lgrp_notif_interfaces[ldev_num][lgrp_num].lgrp_events);
			init_waitqueue_head(&lgrp_notif_interfaces[ldev_num][lgrp_num].wait);
			mutex_init(&lgrp_notif_interfaces[ldev_num][lgrp_num].read_lock);
			spin_lock_init(&lgrp_notif_interfaces[ldev_num][lgrp_num].data_lock);
		}
	}

	return 0;
}

static ssize_t sl_test_lgrp_event_str(struct lgrp_notif_event *event, char *buf, size_t size)
{
	struct sl_lgrp_notif_msg *msg;
	char                      cause_str[SL_STR_MAP_STR_SIZE_DEFAULT];

	msg = &event->msg;

	switch (msg->type) {
	case SL_LGRP_NOTIF_LINK_ERROR:
		return scnprintf(buf, size, "%ptTd %ptTt %u %u %u 0x%llx %s %d\n",
			&event->timestamp,
			&event->timestamp,
			msg->ldev_num,
			msg->lgrp_num,
			msg->link_num,
			msg->info_map,
			sl_lgrp_notif_str(msg->type),
			msg->info.error);
	case SL_LGRP_NOTIF_LINK_UP_FAIL:
	case SL_LGRP_NOTIF_LINK_ASYNC_DOWN:
	case SL_LGRP_NOTIF_LINK_DOWN:
		sl_link_down_cause_map_with_info_str(msg->info.cause_map, cause_str, sizeof(cause_str));
		return scnprintf(buf, size, "%ptTd %ptTt %u %u %u 0x%llx %s %s\n",
			&event->timestamp,
			&event->timestamp,
			msg->ldev_num,
			msg->lgrp_num,
			msg->link_num,
			msg->info_map,
			sl_lgrp_notif_str(msg->type),
			cause_str);
	case SL_LGRP_NOTIF_LINK_UP:
		return scnprintf(buf, size, "%ptTd %ptTt %u %u %u 0x%llx %s %s\n",
			&event->timestamp,
			&event->timestamp,
			msg->ldev_num,
			msg->lgrp_num,
			msg->link_num,
			msg->info_map,
			sl_lgrp_notif_str(msg->type),
			sl_lgrp_config_tech_str(msg->info.link_up.mode));
	default:
		return scnprintf(buf, size, "%ptTd %ptTt %u %u %u 0x%llx %s none\n",
			&event->timestamp,
			&event->timestamp,
			msg->ldev_num,
			msg->lgrp_num,
			msg->link_num,
			msg->info_map,
			sl_lgrp_notif_str(msg->type));
	};
}

static bool sl_test_lgrp_notif_available(struct lgrp_notif_interface *interface)
{
	bool available;

	spin_lock(&interface->data_lock);
	available = !kfifo_is_empty(&interface->lgrp_events);
	spin_unlock(&interface->data_lock);

	return available;
}

static ssize_t sl_test_lgrp_notif_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{
	int                          rtn;
	int                          notif_count;
	ssize_t                      len;
	ssize_t                      bytes;
	struct lgrp_notif_interface *interface;
	struct lgrp_notif_event      event;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"lgrp_notif_read (count = %lu, f_pos = %lld)", count, *f_pos);

	if (count < SL_TEST_EVENT_STR_SIZE) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp_notif_read insufficient space (count = %lu)", count);
		return -ENOMEM;
	}

	interface = filep->private_data;
	if (mutex_lock_interruptible(&interface->read_lock))
		return -ERESTARTSYS;

	while (!sl_test_lgrp_notif_available(interface)) {
		if (filep->f_flags & O_NONBLOCK) {
			mutex_unlock(&interface->read_lock);
			return -EAGAIN;
		}

		if (is_pollout_req(interface))
			wake_up_poll(&interface->wait, EPOLLOUT);

		rtn = wait_event_interruptible(interface->wait,
					       sl_test_lgrp_notif_available(interface));
		if (rtn) {
			mutex_unlock(&interface->read_lock);
			return rtn;
		}
	}

	spin_lock(&interface->data_lock);
	notif_count = kfifo_out_peek(&interface->lgrp_events, &event, 1);
	spin_unlock(&interface->data_lock);
	if (notif_count != 1) {
		mutex_unlock(&interface->read_lock);
		return -ENOENT;
	}

	len = sl_test_lgrp_event_str(&event, interface->event_str,
				     sizeof(interface->event_str));
	if (len < 0) {
		mutex_unlock(&interface->read_lock);
		return len;
	}

	bytes = simple_read_from_buffer(buf, count, f_pos,
					interface->event_str, len + 1);
	if (bytes < 0) {
		mutex_unlock(&interface->read_lock);
		return bytes;
	}

	if (bytes > 0) {
		spin_lock(&interface->data_lock);
		notif_count = kfifo_out(&interface->lgrp_events, &event, 1);
		spin_unlock(&interface->data_lock);
		if (notif_count != 1) {
			mutex_unlock(&interface->read_lock);
			return -ENOENT;
		}
		*f_pos = 0;
	}

	mutex_unlock(&interface->read_lock);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "read complete (bytes = %ld)", bytes);

	return bytes;
}

static __poll_t sl_test_lgrp_notif_poll(struct file *filep, struct poll_table_struct *wait)
{
	struct lgrp_notif_interface *interface;
	bool                         notif_available;

	interface = filep->private_data;

	is_pollout_req_set(interface, poll_requested_events(wait) & EPOLLOUT);
	poll_wait(filep, &interface->wait, wait);

	notif_available = sl_test_lgrp_notif_available(interface);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll (notif_available = %s)",
		   notif_available ? "true" : "false");

	if (notif_available) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll data ready");
		return (EPOLLIN | EPOLLRDNORM);
	}

	if (is_pollout_req(interface)) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll POLLOUT requested");
		return EPOLLOUT;
	}

	return 0;
}

static int sl_test_lgrp_notif_open(struct inode *inode, struct file *filep)
{
	u8 ldev_num;
	u8 lgrp_num;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_open");

	ldev_num = sl_test_debugfs_ldev_num_get();
	lgrp_num = sl_test_debugfs_lgrp_num_get();

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "notif_open (ldev_num = %u, lgrp_num = %u)", ldev_num, lgrp_num);

	filep->private_data = &lgrp_notif_interfaces[ldev_num][lgrp_num];

	return 0;
}

static const struct file_operations sl_test_lgrp_notifs_fops = {

	.owner  = THIS_MODULE,
	.open   = sl_test_lgrp_notif_open,
	.read   = sl_test_lgrp_notif_read,
	.poll   = sl_test_lgrp_notif_poll,
	.llseek = noop_llseek,
};

static int sl_test_lgrp_notif_push(struct sl_lgrp_notif_msg *msg, time64_t timestamp)
{
	struct lgrp_notif_interface *interface;
	struct lgrp_notif_event      event;
	int                          copied;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"lgrp_notif_push (msg = %p, timestamp = %lld", msg, timestamp);

	event.timestamp = timestamp;
	event.msg       = *msg;

	interface = &lgrp_notif_interfaces[msg->ldev_num][msg->lgrp_num];

	copied = kfifo_in_spinlocked(&interface->lgrp_events, &event, 1, &interface->data_lock);
	if (copied != 0) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "waking poll (copied = %d)", copied);
		wake_up_poll(&interface->wait, EPOLLIN);
	} else {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
			"kfifo_in_spinlocked failed (copied = %d)", copied);
		return -ENOSPC;
	}

	return 0;
}

static void sl_test_lgrp_notif_callback(void *tag, struct sl_lgrp_notif_msg *msg)
{
	int            rtn;
	struct sl_lgrp notif_lgrp;

	sl_test_lgrp_init(&notif_lgrp, msg->ldev_num, msg->lgrp_num);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"notif_callback (ldev_num = %u, lgrp_num = %u, type = %u %s)",
		notif_lgrp.ldev_num, notif_lgrp.num, msg->type, sl_lgrp_notif_str(msg->type));

	rtn = sl_test_lgrp_notif_push(msg, ktime_get_real_seconds());
	if (rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_push failed [%d]", rtn);
}

static ssize_t sl_test_lgrp_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
{
	int     rtn;
	int     call_rtn;
	ssize_t len;
	char    cmd_buf[CMD_LEN];

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_cmd_write");

	/* Don't allow partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"partial cmd_write");
		return 0;
	}

	if (!size || size >= sizeof(cmd_buf)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write too big (size = %ld)", size);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(cmd_buf, sizeof(cmd_buf) - 1, pos, buf, size);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write simple_write_to_buffer [%ld]", len);
		return len;
	}

	cmd_buf[len] = '\0';

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "(cmd_buf = %s)", cmd_buf);

	rtn = sl_test_debugfs_cmd_call(cmd_buf, lgrp_cmd_list, ARRAY_SIZE(lgrp_cmd_list), &call_rtn);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cmd_write failed (cmd_buf = %s) [%d]", cmd_buf, rtn);
		return rtn;
	}

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "cmd_write succeeded (cmd_buf = %s, call_rtn = %d)", cmd_buf, call_rtn);
	if (call_rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cmd_write call failed (cmd_buf = %s) [%d]", cmd_buf, call_rtn);
		return call_rtn;
	}

	return size;
}

static const struct file_operations sl_test_lgrp_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_lgrp_cmd_write,
};

static void sl_test_furcation_opts(char *buf, size_t size)
{
	snprintf(buf, size, "%s %s %s\n",
		sl_media_furcation_str(SL_MEDIA_FURCATION_X1),
		sl_media_furcation_str(SL_MEDIA_FURCATION_X2),
		sl_media_furcation_str(SL_MEDIA_FURCATION_X4));
};

static struct str_conv_u32 lgrp_furcation = {
	.to_str = sl_media_furcation_str,
	.to_u32 = sl_test_furcation_from_str,
	.opts   = sl_test_furcation_opts,
	.value  = &lgrp_config.furcation,
};

#define STATIC_OPT_ENTRY(_name, _bit_field) static struct options_field_entry option_##_name = { \
	.options = &lgrp_config.options,                                                         \
	.field   = SL_LGRP_OPT_##_bit_field,                                                     \
}

#define STATIC_CONFIG_OPT_ENTRY(_name, _bit_field) static struct options_field_entry config_option_##_name = { \
	.options = &lgrp_config.options,                                                                       \
	.field   = SL_LGRP_CONFIG_OPT_##_bit_field,                                                            \
}

#define STATIC_POLICY_OPT_ENTRY(_name, _bit_field) static struct options_field_entry policy_option_##_name = { \
	.options = &lgrp_policy.options,                                                                       \
	.field   = SL_LGRP_POLICY_OPT_##_bit_field,                                                            \
}

#define STATIC_FEC_MAP_ENTRY(_name, _bit_field) static struct options_field_entry fec_map_##_name##_set = { \
	.options = &lgrp_config.fec_map,                                                                    \
	.field   = SL_LGRP_CONFIG_FEC_##_bit_field,                                                         \
}

#define STATIC_TECH_MAP_ENTRY(_name, _bit_field) static struct options_field_entry tech_map_##_name##_set = { \
	.options = &lgrp_config.tech_map,                                                                     \
	.field   = SL_LGRP_CONFIG_TECH_##_bit_field,                                                          \
}

STATIC_OPT_ENTRY(lock, LOCK);

STATIC_CONFIG_OPT_ENTRY(fabric_link,     FABRIC);
STATIC_CONFIG_OPT_ENTRY(r1_partner,      R1);
STATIC_CONFIG_OPT_ENTRY(loopback_serdes, LOOPBACK_SERDES_ENABLE);
STATIC_CONFIG_OPT_ENTRY(loopback_host,   LOOPBACK_HOST_ENABLE);
STATIC_CONFIG_OPT_ENTRY(loopback_media,  LOOPBACK_MEDIA_ENABLE);

STATIC_FEC_MAP_ENTRY(rs, RS);
STATIC_FEC_MAP_ENTRY(rs_ll, RS_LL);

STATIC_TECH_MAP_ENTRY(ck400g, CK_400G);
STATIC_TECH_MAP_ENTRY(ck200g, CK_200G);
STATIC_TECH_MAP_ENTRY(ck100g, CK_100G);
STATIC_TECH_MAP_ENTRY(bs200g, BS_200G);
STATIC_TECH_MAP_ENTRY(cd100g, CD_100G);
STATIC_TECH_MAP_ENTRY(cd50g,  CD_50G);
STATIC_TECH_MAP_ENTRY(bj100g, BJ_100G);

static void sl_test_lgrp_config_init(void)
{
	lgrp_config.magic     = SL_LGRP_CONFIG_MAGIC;
	lgrp_config.ver       = SL_LGRP_CONFIG_VER;
	lgrp_config.size      = sizeof(lgrp_config);
	lgrp_config.options  |= SL_LGRP_OPT_ADMIN;
	lgrp_config.mfs       = 0;
	lgrp_config.fec_map   = 0;
	lgrp_config.fec_mode  = 0;
	lgrp_config.tech_map  = 0;
	lgrp_config.furcation = 0;
}

static void sl_test_lgrp_policy_init(void)
{
	lgrp_policy.magic     = SL_LGRP_POLICY_MAGIC;
	lgrp_policy.ver       = SL_LGRP_POLICY_VER;
	lgrp_policy.size      = sizeof(lgrp_policy);
	lgrp_policy.options  |= SL_LGRP_OPT_ADMIN;
}

struct sl_lgrp *sl_test_lgrp_get(void)
{
	lgrp.ldev_num = sl_test_debugfs_ldev_num_get();

	return &lgrp;
}

int sl_test_debugfs_lgrp_create(struct dentry *top_dir)
{
	int            rtn;
	struct dentry *config_dir;
	struct dentry *policies_dir;
	struct dentry *dentry;
	u8             ldev_num;
	char           notifs_file_name[16];

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "debugfs_lgrp_create");

	lgrp_dir = debugfs_create_dir("lgrp", top_dir);
	rtn = sl_test_debugfs_entry_check(lgrp_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "lgrp debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	sl_test_lgrp_init(&lgrp, sl_test_debugfs_ldev_num_get(), 0);
	sl_test_lgrp_notif_setup();

	SL_TEST_DEBUGFS_CREATE(u8, "num", lgrp_dir, &lgrp.num);

	sl_test_lgrp_config_init();

	config_dir = debugfs_create_dir("config", lgrp_dir);
	rtn = sl_test_debugfs_entry_check(config_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "lgrp config debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	SL_TEST_DEBUGFS_CREATE(u32,          "mfs",                 config_dir, &lgrp_config.mfs);
	SL_TEST_DEBUGFS_CREATE(opt,          "fec_map_rs_set",      config_dir, &fec_map_rs_set);
	SL_TEST_DEBUGFS_CREATE(opt,          "fec_map_rs_ll_set",   config_dir, &fec_map_rs_ll_set);
	SL_TEST_DEBUGFS_CREATE(u32,          "fec_mode",            config_dir, &lgrp_config.fec_mode);
	SL_TEST_DEBUGFS_CREATE(opt,          "tech_map_ck400g_set", config_dir, &tech_map_ck400g_set);
	SL_TEST_DEBUGFS_CREATE(opt,          "tech_map_ck200g_set", config_dir, &tech_map_ck200g_set);
	SL_TEST_DEBUGFS_CREATE(opt,          "tech_map_ck100g_set", config_dir, &tech_map_ck100g_set);
	SL_TEST_DEBUGFS_CREATE(opt,          "tech_map_bs200g_set", config_dir, &tech_map_bs200g_set);
	SL_TEST_DEBUGFS_CREATE(opt,          "tech_map_cd100g_set", config_dir, &tech_map_cd100g_set);
	SL_TEST_DEBUGFS_CREATE(opt,          "tech_map_cd50g_set",  config_dir, &tech_map_cd50g_set);
	SL_TEST_DEBUGFS_CREATE(opt,          "tech_map_bj100g_set", config_dir, &tech_map_bj100g_set);
	SL_TEST_DEBUGFS_CREATE(str_conv_u32, "furcation",           config_dir, &lgrp_furcation);
	SL_TEST_DEBUGFS_CREATE(opt,          "lock",                config_dir, &option_lock);
	SL_TEST_DEBUGFS_CREATE(opt,          "fabric_link",         config_dir, &config_option_fabric_link);
	SL_TEST_DEBUGFS_CREATE(opt,          "r1_partner",          config_dir, &config_option_r1_partner);
	SL_TEST_DEBUGFS_CREATE(opt,          "loopback_serdes",     config_dir, &config_option_loopback_serdes);
	SL_TEST_DEBUGFS_CREATE(opt,          "loopback_host",       config_dir, &config_option_loopback_host);
	SL_TEST_DEBUGFS_CREATE(opt,          "loopback_media",      config_dir, &config_option_loopback_media);

	sl_test_lgrp_policy_init();

	policies_dir = debugfs_create_dir("policies", lgrp_dir);
	rtn = sl_test_debugfs_entry_check(policies_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "lgrp policies debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	dentry = debugfs_create_file("cmds", 0644, lgrp_dir, NULL, &sl_test_lgrp_cmds_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmds debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	dentry = debugfs_create_file("cmd", 0644, lgrp_dir, NULL, &sl_test_lgrp_cmd_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmd debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		snprintf(notifs_file_name, sizeof(notifs_file_name), "notifs%u", ldev_num);

		dentry = debugfs_create_file(notifs_file_name, 0644, lgrp_dir,
					     lgrp_notif_interfaces[ldev_num],
					     &sl_test_lgrp_notifs_fops);
		rtn = sl_test_debugfs_entry_check(dentry);
		if (rtn) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				   "%s debugfs_create_file failed [%d]",
				   notifs_file_name, rtn);
			return rtn;
		}
	}

	return 0;
}

u8 sl_test_debugfs_lgrp_num_get(void)
{
	return lgrp.num;
}

/**
 * sl_test_lgrp_new - Create a new link group.
 *
 * Clients create link groups. Register the test sysfs hierarchy after
 * confirming the link group exists.
 *
 * Return: 0 on success, negative error code on failure
 */
int sl_test_lgrp_new(void)
{
	int                  rtn;
	u8                   ldev_num;
	u8                   lgrp_num;
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ldev_num = sl_test_debugfs_ldev_num_get();
	lgrp_num = sl_test_debugfs_lgrp_num_get();
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "lgrp_new (ldev_num = %u, lgrp_num = %u)", ldev_num,
		   lgrp_num);

	ctrl_lgrp = sl_test_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_new ctrl_lgrp_get failed");
		return -ENODEV;
	}

	rtn = sl_test_pdev_pgrp_add(ldev_num, lgrp_num);
	if (rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "sl_test_pdev_pgrp_add failed [%d]", rtn);

	return rtn;
}

static int sl_test_lgrp_notif_callback_unreg(u8 ldev_num, u8 lgrp_num)
{
	int            rtn;
	struct sl_lgrp notif_lgrp;

	if (ldev_num >= SL_ASIC_MAX_LDEVS || lgrp_num >= SL_ASIC_MAX_LGRPS)
		return -EINVAL;

	mutex_lock(&lgrp_notif_reg_lock);
	if (!lgrp_notif_registered[ldev_num][lgrp_num]) {
		mutex_unlock(&lgrp_notif_reg_lock);
		return -ENOENT;
	}

	sl_test_lgrp_init(&notif_lgrp, ldev_num, lgrp_num);
	rtn = sl_lgrp_notif_callback_unreg(&notif_lgrp,
					   sl_test_lgrp_notif_callback, SL_LGRP_NOTIF_ALL);
	if (rtn) {
		mutex_unlock(&lgrp_notif_reg_lock);
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_callback_reg failed [%d]", rtn);
		return rtn;
	}
	lgrp_notif_registered[ldev_num][lgrp_num] = false;

	mutex_unlock(&lgrp_notif_reg_lock);

	return 0;
}

/**
 * sl_test_lgrp_del - Delete a link group.
 *
 * Unregister test notifications. Clients remain owners of the link group object.
 *
 * Return: 0 on success, negative error code on failure
 */
int sl_test_lgrp_del(void)
{
	int                  rtn;
	u8                   ldev_num;
	u8                   lgrp_num;
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ldev_num = sl_test_debugfs_ldev_num_get();
	lgrp_num = sl_test_debugfs_lgrp_num_get();

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_del (ldev_num = %u, lgrp_num = %u)", ldev_num, lgrp_num);

	rtn = sl_test_lgrp_notif_callback_unreg(ldev_num, lgrp_num);
	switch (rtn) {
	case -ENOENT:
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_callback_unreg returned -ENOENT");
		break;
	case 0:
		break;
	default:
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_callback_unreg failed [%d]", rtn);
		return rtn;
	}

	/* Only for logging purposes */
	ctrl_lgrp = sl_test_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (ctrl_lgrp) {
		sl_log_dbg(ctrl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp_del device found (ctrl_lgrp = %p)",
			ctrl_lgrp);
	}

	return 0;
}

int sl_test_lgrp_config_set(void)
{
	int rtn;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_config_set");

	rtn = sl_lgrp_config_set(sl_test_lgrp_get(), &lgrp_config);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_config_set failed [%d]", rtn);
		return rtn;
	}

	switch (lgrp_config.furcation) {
	default:
	case SL_MEDIA_FURCATION_X1:
		sl_test_pg_cfg_set(sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(), 0);
		break;
	case SL_MEDIA_FURCATION_X2:
		sl_test_pg_cfg_set(sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(), 1);
		break;
	case SL_MEDIA_FURCATION_X4:
		sl_test_pg_cfg_set(sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(), 2);
		break;
	}

	return 0;
}

int sl_test_lgrp_policy_set(void)
{
	return sl_lgrp_policy_set(sl_test_lgrp_get(), &lgrp_policy);
}

/* Today we need to track registration because we aren't in control of the link group object that would handle
 * unregistering for us when we delete. sl-test must unregister as part of its cleanup process so we don't
 * retain a stale callback pointer.
 */
int sl_test_lgrp_notif_reg(void)
{
	int rtn;
	u8 ldev_num = sl_test_debugfs_ldev_num_get();
	u8 lgrp_num = sl_test_debugfs_lgrp_num_get();
	struct sl_lgrp notif_lgrp;

	if (ldev_num >= SL_ASIC_MAX_LDEVS || lgrp_num >= SL_ASIC_MAX_LGRPS)
		return -EINVAL;

	mutex_lock(&lgrp_notif_reg_lock);
	if (lgrp_notif_registered[ldev_num][lgrp_num]) {
		mutex_unlock(&lgrp_notif_reg_lock);
		return -EALREADY;
	}
	sl_test_lgrp_init(&notif_lgrp, ldev_num, lgrp_num);
	rtn = sl_lgrp_notif_callback_reg(&notif_lgrp, sl_test_lgrp_notif_callback,
					 SL_LGRP_NOTIF_ALL, NULL);
	if (rtn) {
		mutex_unlock(&lgrp_notif_reg_lock);
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_callback_reg failed [%d]", rtn);
		return rtn;
	}
	lgrp_notif_registered[ldev_num][lgrp_num] = true;
	mutex_unlock(&lgrp_notif_reg_lock);

	return 0;
}

int sl_test_lgrp_notif_unreg(void)
{
	return sl_test_lgrp_notif_callback_unreg(sl_test_debugfs_ldev_num_get(),
						 sl_test_debugfs_lgrp_num_get());
}

void sl_test_lgrp_notif_unregister_all(void)
{
	int            rtn;
	u8             ldev_num;
	u8             lgrp_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
			rtn = sl_test_lgrp_notif_callback_unreg(ldev_num, lgrp_num);
			if (rtn && rtn != -ENOENT)
				sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
					   "lgrp notifier unregister failed "
					   "(ldev_num = %u, lgrp_num = %u) [%d]",
					   ldev_num, lgrp_num, rtn);
		}
	}
}
