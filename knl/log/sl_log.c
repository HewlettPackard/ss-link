// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/sched.h>

#include "sl_asic.h"

#include "sl_log.h"

#include "sl_ldev.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "sl_llr.h"
#include "sl_mac.h"

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_mac.h"
#include "sl_core_llr.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"
#include "sl_ctl_llr.h"
#include "sl_ctl_mac.h"

#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_link.h"
#include "data/sl_core_data_lgrp.h"
#include "data/sl_core_data_ldev.h"

#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"

#define SL_LOG_MSG_LEN              150
#define SL_LOG_NULL_ID_FMT          "%c[-:--:-] ???         "
#define SL_LOG_DFLT_ID_FMT          "%c[-:--:-] 0x%08X  "
#define SL_LOG_LDEV_ID_FMT          "%c[%1u:--:-] ldev%1u       "
#define SL_LOG_LGRP_ID_FMT          "%c[%1u:%02u:-] %-12s"
#define SL_LOG_LINK_ID_FMT          "%c[%1u:%02u:%1u] %-12s"
#define SL_LOG_JACK_ID_FMT          "%c[-:--:-] jack%02u      "
#define SL_LOG_CONTEXT              (in_interrupt() ? '^' : ' ')

static int remaining_chars(int idx)
{
	return max(SL_LOG_MSG_LEN - idx, 0);
}

static void sl_log_msg_create(const char *level, void *ptr, char *msg,
	const char *block, const char *name, const char *text)
{
	struct sl_ldev            *ldev;
	struct sl_lgrp            *lgrp;
	struct sl_link            *link;
	struct sl_llr             *llr;
	struct sl_mac             *mac;
	struct sl_ctl_ldev        *ctl_ldev;
	struct sl_ctl_lgrp        *ctl_lgrp;
	struct sl_ctl_link        *ctl_link;
	struct sl_ctl_llr         *ctl_llr;
	struct sl_ctl_mac         *ctl_mac;
	struct sl_core_ldev       *core_ldev;
	struct sl_core_lgrp       *core_lgrp;
	struct sl_core_link       *core_link;
	struct sl_core_mac        *core_mac;
	struct sl_core_llr        *core_llr;
	struct sl_media_ldev      *media_ldev;
	struct sl_media_lgrp      *media_lgrp;
	struct sl_media_jack      *media_jack;
	int                        idx;

	idx = snprintf(msg, SL_LOG_MSG_LEN, "%s%s", level, module_name(THIS_MODULE));
	if (ptr != NULL) {
		switch (*((u32 *)ptr)) {
		case SL_LDEV_MAGIC:
			ldev = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LDEV_ID_FMT,
				SL_LOG_CONTEXT,
				ldev->num, ldev->num);
			break;
		case SL_LGRP_MAGIC:
			lgrp = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LGRP_ID_FMT,
				SL_LOG_CONTEXT,
				lgrp->ldev_num, lgrp->num, "lgrp");
			break;
		case SL_LINK_MAGIC:
			link = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				link->ldev_num, link->lgrp_num, link->num, "link");
			break;
		case SL_LLR_MAGIC:
			llr = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				llr->ldev_num, llr->lgrp_num, llr->num, "llr");
			break;
		case SL_MAC_MAGIC:
			mac = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				mac->ldev_num, mac->lgrp_num, mac->num, "mac");
			break;
		case SL_CTL_LDEV_MAGIC:
			ctl_ldev = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LDEV_ID_FMT,
				SL_LOG_CONTEXT,
				ctl_ldev->num, ctl_ldev->num);
			break;
		case SL_CTL_LGRP_MAGIC:
			ctl_lgrp = ptr;
			spin_lock(&(ctl_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LGRP_ID_FMT,
				SL_LOG_CONTEXT,
				ctl_lgrp->ctl_ldev->num, ctl_lgrp->num,
				ctl_lgrp->log_connect_id);
			spin_unlock(&(ctl_lgrp->log_lock));
			break;
		case SL_CTL_LINK_MAGIC:
			ctl_link = ptr;
			spin_lock(&(ctl_link->ctl_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
				ctl_link->ctl_lgrp->log_connect_id);
			spin_unlock(&(ctl_link->ctl_lgrp->log_lock));
			break;
		case SL_CTL_LLR_MAGIC:
			ctl_llr = ptr;
			spin_lock(&(ctl_llr->ctl_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num, ctl_llr->num,
				ctl_llr->ctl_lgrp->log_connect_id);
			spin_unlock(&(ctl_llr->ctl_lgrp->log_lock));
			break;
		case SL_CTL_MAC_MAGIC:
			ctl_mac = ptr;
			spin_lock(&(ctl_mac->ctl_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				ctl_mac->ctl_lgrp->ctl_ldev->num, ctl_mac->ctl_lgrp->num, ctl_mac->num,
				ctl_mac->ctl_lgrp->log_connect_id);
			spin_unlock(&(ctl_mac->ctl_lgrp->log_lock));
			break;
		case SL_CORE_LDEV_MAGIC:
			core_ldev = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LDEV_ID_FMT,
				SL_LOG_CONTEXT,
				core_ldev->num, core_ldev->num);
			break;
		case SL_CORE_LGRP_MAGIC:
			core_lgrp = ptr;
			spin_lock(&(core_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LGRP_ID_FMT,
				SL_LOG_CONTEXT,
				core_lgrp->core_ldev->num, core_lgrp->num,
				core_lgrp->log_connect_id);
			spin_unlock(&(core_lgrp->log_lock));
			break;
		case SL_CORE_LINK_MAGIC:
			core_link = ptr;
			spin_lock(&(core_link->core_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num, core_link->num,
				core_link->core_lgrp->log_connect_id);
			spin_unlock(&(core_link->core_lgrp->log_lock));
			break;
		case SL_CORE_MAC_MAGIC:
			core_mac = ptr;
			spin_lock(&(core_mac->core_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				core_mac->core_lgrp->core_ldev->num, core_mac->core_lgrp->num, core_mac->num,
				core_mac->core_lgrp->log_connect_id);
			spin_unlock(&(core_mac->core_lgrp->log_lock));
			break;
		case SL_CORE_LLR_MAGIC:
			core_llr = ptr;
			spin_lock(&(core_llr->core_lgrp->log_lock));
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LINK_ID_FMT,
				SL_LOG_CONTEXT,
				core_llr->core_lgrp->core_ldev->num, core_llr->core_lgrp->num, core_llr->num,
				core_llr->core_lgrp->log_connect_id);
			spin_unlock(&(core_llr->core_lgrp->log_lock));
			break;
		case SL_MEDIA_LDEV_MAGIC:
			media_ldev = ptr;
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LDEV_ID_FMT,
				SL_LOG_CONTEXT,
				media_ldev->num, media_ldev->num);
			break;
		case SL_MEDIA_LGRP_MAGIC:
			media_lgrp = ptr;
			spin_lock(&media_lgrp->log_lock);
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_LGRP_ID_FMT,
				SL_LOG_CONTEXT,
				media_lgrp->media_ldev->num, media_lgrp->num,
				media_lgrp->connect_id);
			spin_unlock(&media_lgrp->log_lock);
			break;
		case SL_MEDIA_JACK_MAGIC:
			media_jack = ptr;
			spin_lock(&media_jack->log_lock);
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_JACK_ID_FMT,
				SL_LOG_CONTEXT,
				media_jack->physical_num);
			spin_unlock(&media_jack->log_lock);
			break;
		default:
			idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_DFLT_ID_FMT,
				SL_LOG_CONTEXT,
				*((u32 *)ptr));
			break;
		}
	} else {
		idx += snprintf(msg + idx, remaining_chars(idx), SL_LOG_NULL_ID_FMT,
			SL_LOG_CONTEXT);
	}

	snprintf(msg + idx, remaining_chars(idx), " %5s-%-10s: %s\n", block, name, text);
}

void sl_log(void *ptr, const char *level, const char *block,
	const char *name, const char *text, ...)
{
	char    msg[SL_LOG_MSG_LEN + 1];
	va_list args;

	if (!level) {
		pr_err("NULL level\n");
		return;
	}
	if (!block) {
		pr_err("NULL block\n");
		return;
	}
	if (!name) {
		pr_err("NULL name\n");
		return;
	}
	if (!text) {
		pr_err("NULL text\n");
		return;
	}

	sl_log_msg_create(level, ptr, msg, block, name, text);

	va_start(args, text);
	vprintk(msg, args);
	va_end(args);
}

void sl_log_err_trace(void *ptr, const char *block,
	const char *name, const char *text, ...)
{
	char                 msg[SL_LOG_MSG_LEN + 1];
	va_list              args;
	struct sl_ctl_lgrp   *ctl_lgrp;
	struct sl_ctl_link   *ctl_link;
	struct sl_ctl_llr    *ctl_llr;
	struct sl_ctl_mac    *ctl_mac;
	struct sl_core_lgrp  *core_lgrp;
	struct sl_core_link  *core_link;
	struct sl_core_mac   *core_mac;
	struct sl_core_llr   *core_llr;
	struct sl_media_lgrp *media_lgrp;

	if (!block) {
		pr_err("NULL block\n");
		return;
	}
	if (!name) {
		pr_err("NULL name\n");
		return;
	}
	if (!text) {
		pr_err("NULL text\n");
		return;
	}

	if (ptr == NULL)
		return;

	switch (*((u32 *)ptr)) {
	case SL_CTL_LGRP_MAGIC:
		ctl_lgrp = ptr;
		if (!ctl_lgrp->err_trace_enable)
			return;
		break;
	case SL_CTL_LINK_MAGIC:
		ctl_link = ptr;
		if (!ctl_link->ctl_lgrp->err_trace_enable)
			return;
		break;
	case SL_CTL_LLR_MAGIC:
		ctl_llr = ptr;
		if (!ctl_llr->ctl_lgrp->err_trace_enable)
			return;
		break;
	case SL_CTL_MAC_MAGIC:
		ctl_mac = ptr;
		if (!ctl_mac->ctl_lgrp->err_trace_enable)
			return;
		break;
	case SL_CORE_LGRP_MAGIC:
		core_lgrp = ptr;
		if (!core_lgrp->err_trace_enable)
			return;
		break;
	case SL_CORE_LINK_MAGIC:
		core_link = ptr;
		if (!core_link->core_lgrp->err_trace_enable)
			return;
		break;
	case SL_CORE_MAC_MAGIC:
		core_mac = ptr;
		if (!core_mac->core_lgrp->err_trace_enable)
			return;
		break;
	case SL_CORE_LLR_MAGIC:
		core_llr = ptr;
		if (!core_llr->core_lgrp->err_trace_enable)
			return;
		break;
	case SL_MEDIA_LGRP_MAGIC:
		media_lgrp = ptr;
		if (!media_lgrp->err_trace_enable)
			return;
		break;
	default:
		return;
	}

	sl_log_msg_create(KERN_ERR, ptr, msg, block, name, text);

	va_start(args, text);
	vprintk(msg, args);
	va_end(args);
}

void sl_log_warn_trace(void *ptr, const char *block,
	const char *name, const char *text, ...)
{
	char                 msg[SL_LOG_MSG_LEN + 1];
	va_list              args;
	struct sl_ctl_lgrp   *ctl_lgrp;
	struct sl_ctl_link   *ctl_link;
	struct sl_ctl_llr    *ctl_llr;
	struct sl_ctl_mac    *ctl_mac;
	struct sl_core_lgrp  *core_lgrp;
	struct sl_core_link  *core_link;
	struct sl_core_mac   *core_mac;
	struct sl_core_llr   *core_llr;
	struct sl_media_lgrp *media_lgrp;

	if (!block) {
		pr_err("NULL block\n");
		return;
	}
	if (!name) {
		pr_err("NULL name\n");
		return;
	}
	if (!text) {
		pr_err("NULL text\n");
		return;
	}

	if (ptr == NULL)
		return;

	switch (*((u32 *)ptr)) {
	case SL_CTL_LGRP_MAGIC:
		ctl_lgrp = ptr;
		if (!ctl_lgrp->warn_trace_enable)
			return;
		break;
	case SL_CTL_LINK_MAGIC:
		ctl_link = ptr;
		if (!ctl_link->ctl_lgrp->warn_trace_enable)
			return;
		break;
	case SL_CTL_LLR_MAGIC:
		ctl_llr = ptr;
		if (!ctl_llr->ctl_lgrp->warn_trace_enable)
			return;
		break;
	case SL_CTL_MAC_MAGIC:
		ctl_mac = ptr;
		if (!ctl_mac->ctl_lgrp->warn_trace_enable)
			return;
		break;
	case SL_CORE_LGRP_MAGIC:
		core_lgrp = ptr;
		if (!core_lgrp->warn_trace_enable)
			return;
		break;
	case SL_CORE_LINK_MAGIC:
		core_link = ptr;
		if (!core_link->core_lgrp->warn_trace_enable)
			return;
		break;
	case SL_CORE_MAC_MAGIC:
		core_mac = ptr;
		if (!core_mac->core_lgrp->warn_trace_enable)
			return;
		break;
	case SL_CORE_LLR_MAGIC:
		core_llr = ptr;
		if (!core_llr->core_lgrp->warn_trace_enable)
			return;
		break;
	case SL_MEDIA_LGRP_MAGIC:
		media_lgrp = ptr;
		if (!media_lgrp->warn_trace_enable)
			return;
		break;
	default:
		return;
	}

	sl_log_msg_create(KERN_WARNING, ptr, msg, block, name, text);

	va_start(args, text);
	vprintk(msg, args);
	va_end(args);
}

void __sl_log_dynamic_dbg(struct _ddebug *desc, void *ptr, const char *block,
	const char *name, const char *text, ...)
{
	char    msg[SL_LOG_MSG_LEN + 1];
	va_list args;

	if (!desc) {
		pr_err("NULL desc\n");
		return;
	}
	if (!block) {
		pr_err("NULL desc\n");
		return;
	}
	if (!name) {
		pr_err("NULL name\n");
		return;
	}
	if (!text) {
		pr_err("NULL text\n");
		return;
	}

	sl_log_msg_create(KERN_DEBUG, ptr, msg, block, name, text);

	va_start(args, text);
	vprintk(msg, args);
	va_end(args);
}
