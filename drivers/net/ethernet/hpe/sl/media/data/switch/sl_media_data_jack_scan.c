// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#include <linux/hpe/sl/sl_media.h>
#include <linux/hsnxcvr-api.h>

#include "sl_asic.h"
#include "base/sl_media_log.h"
#include "base/sl_media_eeprom.h"
#include "sl_media_jack.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_ldev.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_cable_db_ops.h"

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

void sl_media_data_jack_insert_start_work(struct work_struct *work)
{
	struct sl_media_jack *media_jack;

	media_jack = container_of(work, struct sl_media_jack, insert_work);

	sl_media_log_dbg(media_jack->media_ldev, LOG_NAME, "insert start work (jack_num = %u)", media_jack->num);

	sl_media_data_jack_insert(media_jack);
}

void sl_media_data_jack_work_init(struct sl_media_jack *media_jack)
{
	sl_media_log_dbg(media_jack->media_ldev, LOG_NAME, "work init (jack_num = %u)", media_jack->num);

	INIT_WORK(&media_jack->insert_work, sl_media_data_jack_insert_start_work);
}

static struct notifier_block event_notifier = {
	.notifier_call = sl_media_data_jack_event,
	.priority = 0,
};

/* max wait of 60s */
#define SL_MEDIA_JACK_SCAN_DONE_TRIES_WAIT_MS 20
#define SL_MEDIA_JACK_SCAN_DONE_TRIES         (60000 / SL_MEDIA_JACK_SCAN_DONE_TRIES_WAIT_MS)
static int sl_media_data_jack_scan_done_check(struct sl_media_ldev *media_ldev, u32 done_check)
{
	u32 done_tries;

	sl_media_log_dbg(media_ldev, LOG_NAME,
			 "scan done (done_check = 0x%X)", done_check);
	done_tries = 0;
	while ((sl_media_data_jack_insert_status_map_get(media_ldev->num) & done_check) != done_check) {
		sl_media_log_dbg(media_ldev, LOG_NAME,
				 "scan done (map = 0x%X)", sl_media_data_jack_insert_status_map_get(media_ldev->num));
		if (done_tries++ >= SL_MEDIA_JACK_SCAN_DONE_TRIES) {
			sl_media_log_dbg(media_ldev, LOG_NAME,
					 "scan done failed (map = 0x%X)", sl_media_data_jack_insert_status_map_get(media_ldev->num));
			return -EIO;
		}
		msleep(SL_MEDIA_JACK_SCAN_DONE_TRIES_WAIT_MS);
	}

	return 0;
}

int sl_media_data_jack_scan(struct sl_media_ldev *media_ldev)
{
	u8                          jack_count;
	struct sl_media_jack       *media_jack;
	struct sl_media_attr        media_attr;
	void                       *hdl;
	int                         rtn;
	int                         ret;
	struct xcvr_jack_data_v3    jack_data;
	struct xcvr_status_data     status_data;
	u32                         done_check;
	u8                          physical_jack_num;
	int                         x;

	sl_media_log_dbg(media_ldev, LOG_NAME, "scan (max_jack_num = %u)", SL_MEDIA_MAX_JACK_NUM);

	sl_media_data_jack_insert_status_map_init(media_ldev->num);

	sl_media_data_jack_event_ignore_set(true);

	rtn = register_hsnxcvr_notifier(&event_notifier);
	if (rtn) {
		sl_media_log_warn(media_ldev, LOG_NAME, "scan register jack event notifier failed [%d]", rtn);
// FIXME: this seems bad
		rtn = 0;
		goto out;
	}

	jack_count = 0;
	done_check = 0;
	hdl = hsnxcvr_get_next_hdl(NULL);
	while (hdl && jack_count < SL_MEDIA_MAX_JACK_NUM) {
		media_jack = sl_media_data_jack_get(media_ldev->num, jack_count);

		media_jack->hdl = hdl;

		rtn = hsnxcvr_jack_v3_get(media_jack->hdl, &jack_data);
		if (rtn) {
			sl_media_log_err(media_ldev, LOG_NAME,
					 "insert jack_get failed [%d] (jack_num = %u)", rtn, media_jack->num);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_JACK_GET);
			sl_media_data_jack_led_set(media_jack);
			memset(&media_attr, 0, sizeof(struct sl_media_attr));
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			ret = sl_media_data_jack_cable_attr_set(media_jack, &media_attr);
			if (ret)
				sl_media_log_warn_trace(media_ldev, LOG_NAME,
							"insert cable_attr_set failed [%d] (jack_num = %u)",
							ret, media_jack->num);
			sl_media_data_jack_cable_attr_send(media_jack);
			goto next;
		}
		media_jack->jack_type  = jack_data.jack_type;
		media_jack->port_count = jack_data.port_count;
		if (jack_data.port_count > SL_MEDIA_MAX_LGRPS_PER_JACK)
			sl_media_log_warn(media_ldev, LOG_NAME,
					  "insert port_count invalid (actual = %u, limit = %u)",
					  jack_data.port_count, SL_MEDIA_MAX_LGRPS_PER_JACK);
		memcpy(&(media_jack->asic_port), &(jack_data.asic_port), sizeof(jack_data.asic_port));

		rtn = kstrtou8(jack_data.name + 1, 10, &physical_jack_num);
		if (rtn) {
			sl_media_log_err(media_ldev, LOG_NAME,
					 "insert kstrtou8 failed [%d] (jack_num = %u)", rtn, media_jack->num);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_JACK_GET);
			sl_media_data_jack_led_set(media_jack);
			memset(&media_attr, 0, sizeof(struct sl_media_attr));
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			media_jack->physical_num = 999;
			ret = sl_media_data_jack_cable_attr_set(media_jack, &media_attr);
			if (ret)
				sl_media_log_warn_trace(media_ldev, LOG_NAME,
							"insert cable_attr_set failed [%d] (jack_num = %u)",
							ret, media_jack->num);
			sl_media_data_jack_cable_attr_send(media_jack);
			goto next;
		}
		media_jack->physical_num = physical_jack_num;

		sl_media_log_dbg(media_ldev, LOG_NAME,
				 "scan (jack_num = %u, jack_type = %u, port_count = %u)",
				 media_jack->num, media_jack->jack_type, media_jack->port_count);

		rtn = hsnxcvr_status_get(media_jack->hdl, &status_data);
		if (rtn) {
			sl_media_log_err(media_ldev, LOG_NAME,
					 "scan status_get failed (jack_num = %u)", jack_count);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_JACK_STATUS_GET);
			sl_media_data_jack_led_set(media_jack);
			memset(&media_attr, 0, sizeof(struct sl_media_attr));
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			rtn = sl_media_data_jack_cable_attr_set(media_jack, &media_attr);
			if (rtn)
				sl_media_log_warn_trace(media_ldev, LOG_NAME,
							"scan cable_attr_set failed (jack_num = %u) [%d]",
							jack_count, rtn);
			sl_media_data_jack_cable_attr_send(media_jack);
			goto next;
		}
		media_jack->status = status_data.flags;

		sl_media_log_dbg(media_ldev, LOG_NAME,
				 "scan (jack_num = %u, status = 0x%X)",
				 jack_count, media_jack->status);

		if ((status_data.flags & (XCVR_PRESENT | XCVR_JACK_POWERED)) == (XCVR_PRESENT | XCVR_JACK_POWERED)) {
			sl_media_data_jack_insert_status_map_clr(media_ldev->num, media_jack->num);
			done_check |= BIT(jack_count);
			queue_work(media_ldev->workqueue, &media_jack->insert_work);
		} else {
			sl_media_log_info(media_jack, LOG_NAME,
					  "no cable (status = 0x%X)", status_data.flags);
		}

next:

		jack_count++;

		hdl = hsnxcvr_get_next_hdl(hdl);
	}

	rtn = sl_media_data_jack_scan_done_check(media_ldev, done_check);
	if (rtn) {
		sl_media_log_warn(media_ldev, LOG_NAME, "scan done check failed [%d]", rtn);
// FIXME: what else to do here
		goto out;
	}

	/* double check status */
	done_check = 0;
	for (x = 0; x < jack_count; ++x) {
		media_jack = sl_media_data_jack_get(media_ldev->num, x);

		rtn = hsnxcvr_status_get(media_jack->hdl, &status_data);
		if (rtn) {
			sl_media_log_err(media_ldev, LOG_NAME,
					 "scan status_get failed (jack_num = %u)", x);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_JACK_STATUS_GET);
			sl_media_data_jack_led_set(media_jack);
			memset(&media_attr, 0, sizeof(struct sl_media_attr));
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			rtn = sl_media_data_jack_cable_attr_set(media_jack, &media_attr);
			if (rtn)
				sl_media_log_warn_trace(media_ldev, LOG_NAME,
							"scan cable_attr_set failed (jack_num = %u) [%d]", x, rtn);
			sl_media_data_jack_cable_attr_send(media_jack);
			continue;
		}

		if ((status_data.flags & (XCVR_PRESENT | XCVR_JACK_POWERED)) == (XCVR_PRESENT | XCVR_JACK_POWERED)) {
			if ((media_jack->status & (XCVR_PRESENT | XCVR_JACK_POWERED)) == (XCVR_PRESENT | XCVR_JACK_POWERED)) {
				continue;
			} else {
				media_jack->status = status_data.flags;
				sl_media_data_jack_insert_status_map_clr(media_ldev->num, media_jack->num);
				done_check |= BIT(x);
				queue_work(media_ldev->workqueue, &media_jack->insert_work);
			}
		} else {
			if ((media_jack->status & (XCVR_PRESENT | XCVR_JACK_POWERED)) != (XCVR_PRESENT | XCVR_JACK_POWERED)) {
				continue;
			} else {
				sl_media_data_jack_remove(media_jack);
				sl_media_log_info(media_jack, LOG_NAME,
						  "no cable (status = 0x%X)", status_data.flags);
			}
		}
	}

	rtn = sl_media_data_jack_scan_done_check(media_ldev, done_check);
	if (rtn) {
		sl_media_log_warn(media_ldev, LOG_NAME, "scan done check failed [%d]", rtn);
// FIXME: what else to do here
		goto out;
	}

	rtn = 0;

out:

	sl_media_data_jack_event_ignore_set(false);

	sl_media_log_dbg(media_ldev, LOG_NAME, "scan done");

	return rtn;
}

void sl_media_data_jack_unregister_event_notifier(void)
{
	unregister_hsnxcvr_notifier(&event_notifier);
}
