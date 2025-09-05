// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/kobject.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/timekeeping.h>

#include <linux/sl_lgrp.h>
#include <linux/sl_media.h>

#include "sl_asic.h"
#include "sl_lgrp.h"
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
static struct kobject        *lgrp_port_dir[SL_ASIC_MAX_LGRPS];

struct lgrp_notif_event {
	struct sl_lgrp_notif_msg msg;
	time64_t                 timestamp;
};

struct lgrp_notif_interface {
	struct mutex      read_lock;
	spinlock_t        data_lock;
	bool              is_pollout_req;
	wait_queue_head_t wait;
	DECLARE_KFIFO(lgrp_events, struct lgrp_notif_event, SL_TEST_LGRP_NOTIF_NUM_ENTRIES);
};

static struct lgrp_notif_interface lgrp_notif_interfaces[SL_ASIC_MAX_LGRPS];

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
	[LGRP_NEW_CMD]          = { .cmd = "lgrp_new",          .desc = "create lgrp object"                 },
	[LGRP_DEL_CMD]          = { .cmd = "lgrp_del",          .desc = "delete lgrp object"                 },
	[LGRP_CONFIG_WRITE_CMD] = { .cmd = "lgrp_config_write", .desc = "write out the lgrp config from dir" },
	[LGRP_POLICY_WRITE_CMD] = { .cmd = "lgrp_policy_write", .desc = "write out the lgrp policy from dir" },
	[LGRP_NOTIFS_REG_CMD]   = { .cmd = "lgrp_notifs_reg",   .desc = "register for all notifications"     },
	[LGRP_NOTIFS_UNREG_CMD] = { .cmd = "lgrp_notifs_unreg", .desc = "unregister for all notifications"   },
};

#define LGRP_CMD_MATCH(_index, _str) \
	(strncmp((_str), lgrp_cmd_list[_index].cmd, strlen(lgrp_cmd_list[_index].cmd)) == 0)

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
	u8 lgrp_num;

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
		INIT_KFIFO(lgrp_notif_interfaces[lgrp_num].lgrp_events);
		init_waitqueue_head(&lgrp_notif_interfaces[lgrp_num].wait);
		mutex_init(&lgrp_notif_interfaces[lgrp_num].read_lock);
		spin_lock_init(&lgrp_notif_interfaces[lgrp_num].data_lock);
	}

	return 0;
}

static ssize_t sl_test_lgrp_event_str(struct lgrp_notif_event *event, char *buf, size_t size)
{
	struct sl_lgrp_notif_msg *msg;
	char                      cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

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

static char event_str[SL_TEST_EVENT_STR_SIZE];
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

	memset(&event_str, 0, sizeof(event_str));

	do {
		if (kfifo_is_empty(&interface->lgrp_events)) {

			sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
				"lgrp_notif_read empty fifio (f_flags = %u)", filep->f_flags);

			if (filep->f_flags & O_NONBLOCK) {
				sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
					"lgrp_notif_read empty fifo");
				return -EAGAIN;
			}

			if (is_pollout_req(interface)) {
				sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_read POLLOUT");
				wake_up_poll(&interface->wait, EPOLLOUT);
				sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_read waiting");
			}

			rtn = wait_event_interruptible(interface->wait, !kfifo_is_empty(&interface->lgrp_events));
			if (rtn) {
				sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
					"lgrp_notif_read wait failed [%d]", rtn);
				return rtn;
			}
		} else {
			sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_read fifo has data");
		}

		if (mutex_lock_interruptible(&interface->read_lock)) {
			sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
				"lgrp_notif_read mutex lock taken");
			return -ERESTARTSYS;
		}

		notif_count = kfifo_out_spinlocked(&interface->lgrp_events, &event, 1, &interface->data_lock);
		if (notif_count != 1) {
			mutex_unlock(&interface->read_lock);
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"lgrp_notif_read kfifo_get failed (notif_count = %d)", notif_count);
			return -ENOENT;
		}

		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp_notif_read (notif_count = %d)", notif_count);

		len = sl_test_lgrp_event_str(&event, event_str, sizeof(event_str));
		if (len < 0) {
			mutex_unlock(&interface->read_lock);
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"lgrp_notif_read lgrp_event_str failed [%ld]", len);
			return len;
		}

		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "len = %ld", len);
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "event_str: %s", event_str);

		bytes = simple_read_from_buffer(buf, count, f_pos, event_str, len + 1);
		if (bytes < 0) {
			mutex_unlock(&interface->read_lock);
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"lgrp_notif_read lgrp_event_str failed [%ld]", bytes);
			return bytes;
		}

		/* Overwrite user buffer */
		*f_pos = 0;

		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "simple_read_from_buffer (bytes = %ld)", bytes);

		mutex_unlock(&interface->read_lock);

		if (bytes == 0 && (filep->f_flags & O_NONBLOCK))
			return -EAGAIN;

	} while (bytes == 0);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "read complete (bytes = %ld)", bytes);

	return bytes;
}

static __poll_t sl_test_lgrp_notif_poll(struct file *filep, struct poll_table_struct *wait)
{
	struct lgrp_notif_interface *interface;
	bool                         is_empty;

	interface = filep->private_data;

	is_pollout_req_set(interface, poll_requested_events(wait) & EPOLLOUT);

	is_empty = kfifo_is_empty(&interface->lgrp_events);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll (empty = %s)", is_empty ? "true" : "false");

	if (!is_empty) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll data ready");
		return (EPOLLIN | EPOLLRDNORM);
	}

	if (is_pollout_req(interface)) {
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll POLLOUT requested");
		return EPOLLOUT;
	}

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll waiting");
	poll_wait(filep, &interface->wait, wait);
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_poll wait complete");

	return 0;
}

static int sl_test_lgrp_notif_open(struct inode *inode, struct file *filep)
{
	u8 lgrp_num;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_open");

	lgrp_num = sl_test_debugfs_lgrp_num_get();
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "notif_open (lgrp_num = %u", lgrp_num);

	filep->private_data = &lgrp_notif_interfaces[lgrp_num];

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

	interface = &lgrp_notif_interfaces[msg->lgrp_num];

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
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_push failed [%d]", rtn);
}

static ssize_t sl_test_lgrp_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
{
	int     rtn;
	ssize_t len;
	char    cmd_buf[CMD_LEN];
	bool    match_found;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_cmd_write");

	/* Don't allow partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"partial cmd_write");
		return 0;
	}

	if (size > sizeof(cmd_buf)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write too big (size = %ld)", size);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(cmd_buf, sizeof(cmd_buf), pos, buf, size);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write simple_write_to_buffer [%ld]", len);
		return len;
	}

	cmd_buf[len] = '\0';

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "(cmd_buf = %s)", cmd_buf);

	match_found = LGRP_CMD_MATCH(LGRP_NEW_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_lgrp_new();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_lgrp_new failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LGRP_CMD_MATCH(LGRP_DEL_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_lgrp_del();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_lgrp_del failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LGRP_CMD_MATCH(LGRP_CONFIG_WRITE_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_lgrp_config_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_lgrp_config_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LGRP_CMD_MATCH(LGRP_POLICY_WRITE_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_lgrp_policy_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_lgrp_policy_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LGRP_CMD_MATCH(LGRP_NOTIFS_REG_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_lgrp_notif_reg();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_lgrp_notif_reg failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LGRP_CMD_MATCH(LGRP_NOTIFS_UNREG_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_lgrp_notif_unreg();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_lgrp_notif_unreg failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
		"cmd_write no cmd found (cmd_buf = %s)", cmd_buf);

	return -EBADRQC;
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
STATIC_CONFIG_OPT_ENTRY(serdes_loopback, SERDES_LOOPBACK_ENABLE);

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
	lgrp_policy.size      = sizeof(lgrp_config);
	lgrp_policy.options  |= SL_LGRP_OPT_ADMIN;
}

struct sl_lgrp *sl_test_lgrp_get(void)
{
	lgrp.ldev_num = sl_test_debugfs_ldev_num_get();

	return &lgrp;
}

#define SL_TEST_FEC_MAP_SET_NODE(_name)                                                                             \
	do {                                                                                                        \
		rtn = sl_test_debugfs_create_opt("fec_map_"#_name"_set", 0644, config_dir, &fec_map_##_name##_set); \
		if (rtn) {                                                                                          \
			sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,                                                 \
				"link config fec map %s set debugfs_create_opt failed", #_name);                    \
			return -ENOMEM;                                                                             \
		}                                                                                                   \
	} while (0)

#define SL_TEST_TECH_MAP_SET_NODE(_name)                                                                              \
	do {                                                                                                          \
		rtn = sl_test_debugfs_create_opt("tech_map_"#_name"_set", 0644, config_dir, &tech_map_##_name##_set); \
		if (rtn) {                                                                                            \
			sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,                                                   \
				"link config tech map %s set debugfs_create_opt failed", #_name);                     \
			return -ENOMEM;                                                                               \
		}                                                                                                     \
	} while (0)

int sl_test_debugfs_lgrp_create(struct dentry *top_dir)
{
	int            rtn;
	struct dentry *config_dir;
	struct dentry *policies_dir;
	struct dentry *dentry;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "debugfs_lgrp_create");

	lgrp_dir = debugfs_create_dir("lgrp", top_dir);
	if (!lgrp_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_lgrp_init(&lgrp, sl_test_debugfs_ldev_num_get(), 0);
	sl_test_lgrp_notif_setup();

	debugfs_create_u8("num", 0644, lgrp_dir, &lgrp.num);

	sl_test_lgrp_config_init();

	config_dir = debugfs_create_dir("config", lgrp_dir);
	if (!config_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp config debugfs_create_dir failed");
		return -ENOMEM;
	}

	debugfs_create_u32("mfs", 0644, config_dir, &lgrp_config.mfs);

	SL_TEST_FEC_MAP_SET_NODE(rs);
	SL_TEST_FEC_MAP_SET_NODE(rs_ll);

	debugfs_create_u32("fec_mode", 0644, config_dir, &lgrp_config.fec_mode);

	SL_TEST_TECH_MAP_SET_NODE(ck400g);
	SL_TEST_TECH_MAP_SET_NODE(ck200g);
	SL_TEST_TECH_MAP_SET_NODE(ck100g);
	SL_TEST_TECH_MAP_SET_NODE(bs200g);
	SL_TEST_TECH_MAP_SET_NODE(cd100g);
	SL_TEST_TECH_MAP_SET_NODE(cd50g);
	SL_TEST_TECH_MAP_SET_NODE(bj100g);

	sl_test_debugfs_create_str_conv_u32("furcation", 0644, config_dir, &lgrp_furcation);

	rtn = sl_test_debugfs_create_opt("lock", 0644, config_dir, &option_lock);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp config lock debugfs_create_file failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("fabric_link", 0644, config_dir, &config_option_fabric_link);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp config fabric_link debugfs_create_file failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("r1_partner", 0644, config_dir, &config_option_r1_partner);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp config r1_partner debugfs_create_file failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("serdes_loopback", 0644, config_dir, &config_option_serdes_loopback);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp config serdes_loopback debugfs_create_file failed");
		return -ENOMEM;
	}

	sl_test_lgrp_policy_init();

	policies_dir = debugfs_create_dir("policies", lgrp_dir);
	if (!policies_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"lgrp policies debugfs_create_dir failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmds", 0644, lgrp_dir, NULL, &sl_test_lgrp_cmds_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmds debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmd", 0644, lgrp_dir, NULL, &sl_test_lgrp_cmd_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("notifs", 0644, lgrp_dir, NULL, &sl_test_lgrp_notifs_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"notifs debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}

u8 sl_test_debugfs_lgrp_num_get(void)
{
	return lgrp.num;
}

static int sl_test_port_sysfs_init(u8 lgrp_num)
{
	struct kobject *kobj;

	if (lgrp_port_dir[lgrp_num])
		return 0;

	kobj = sl_test_lgrp_kobj_get(sl_test_debugfs_ldev_num_get(), lgrp_num);

	lgrp_port_dir[lgrp_num] = kobject_create_and_add("test_port", kobj);
	if (!lgrp_port_dir[lgrp_num]) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd kobject_create_and_add failed\n");
		return -ENOMEM;
	}

	return 0;
}

static void sl_test_port_sysfs_remove(u8 lgrp_num)
{
	if (lgrp_port_dir[lgrp_num]) {
		kobject_put(lgrp_port_dir[lgrp_num]);
		lgrp_port_dir[lgrp_num] = NULL;
	}
}

void sl_test_port_sysfs_exit(u8 ldev_num)
{
	u8 lgrp_num;
	u8 link_num;

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
		for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
			sl_test_llr_remove(ldev_num, lgrp_num, link_num);
			sl_test_mac_remove(ldev_num, lgrp_num, link_num);
			sl_test_link_remove(ldev_num, lgrp_num, link_num);
		}

		sl_test_port_sysfs_remove(lgrp_num);
	}
}

struct kobject *sl_test_port_sysfs_kobj_get(u8 lgrp_num)
{
	return lgrp_port_dir[lgrp_num];
}

int sl_test_lgrp_new(void)
{
	int                 rtn;
	u8                  lgrp_num;
	struct sl_ctl_lgrp *ctl_lgrp;

	lgrp_num = sl_test_debugfs_lgrp_num_get();
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_new (lgrp_num = %u)", lgrp_num);

	ctl_lgrp = sl_test_ctl_lgrp_get(sl_test_debugfs_ldev_num_get(), lgrp_num);
	if (!ctl_lgrp) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_new ctl_lgrp_get failed");
		return -ENODEV;
	}

	rtn = sl_test_port_sysfs_init(lgrp_num);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_port_sysfs_init failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_test_lgrp_del(void)
{
	u8                  lgrp_num;
	struct sl_ctl_lgrp *ctl_lgrp;

	lgrp_num = sl_test_debugfs_lgrp_num_get();
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "lgrp_del (lgrp_num = %u)", lgrp_num);

	sl_test_port_sysfs_remove(lgrp_num);

	/* Currently we don't want to delete the link group as the client driver created it */
	ctl_lgrp = sl_test_ctl_lgrp_get(sl_test_debugfs_ldev_num_get(), lgrp_num);
	if (ctl_lgrp) {
		sl_log_dbg(ctl_lgrp, LOG_BLOCK, LOG_NAME, "lgrp_del device found (ctl_lgrp = %p)",
			ctl_lgrp);
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

int sl_test_lgrp_notif_reg(void)
{
	int rtn;

	rtn = sl_lgrp_notif_callback_reg(sl_test_lgrp_get(), sl_test_lgrp_notif_callback, SL_LGRP_NOTIF_ALL, NULL);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_callback_reg failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int sl_test_lgrp_notif_unreg(void)
{
	int rtn;

	rtn = sl_lgrp_notif_callback_unreg(sl_test_lgrp_get(), sl_test_lgrp_notif_callback, SL_LGRP_NOTIF_ALL);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "lgrp_notif_callback_reg failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
