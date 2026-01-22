// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/string.h>

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
#include "sl_core_link.h"
#include "sl_ctrl_link_priv.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_lgrp_notif.h"

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

#define FLAT_MEM_OFFSET 2
#define FLAT_MEM_BIT    7
static int sl_media_data_jack_eeprom_page1_get(struct sl_media_jack *media_jack)
{
	int                  rtn;
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "eeprom page1 get");

	if ((media_jack->eeprom_page0[FLAT_MEM_OFFSET] & BIT(FLAT_MEM_BIT)) != 0) {
		sl_media_log_dbg(media_jack, LOG_NAME, "no page1 in eeprom");
		return 0;
	}

	i2c_data.addr   = 0;
	i2c_data.page   = 1;
	i2c_data.bank   = 0;
	i2c_data.offset = 0;
	i2c_data.len    = 256;

	rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_EEPROM_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "i2c read for eeprom page1 failed [%d]", rtn);
		return rtn;
	}
	memcpy(media_jack->eeprom_page1, i2c_data.data, SL_MEDIA_EEPROM_PAGE_SIZE);

	return 0;
}

/*
 * To deal with backplane jack numbers
 */
static inline u8 sl_media_data_jack_num_update(u8 physical_jack_num)
{
	if (physical_jack_num >= 100)
		return physical_jack_num - 76;

	return physical_jack_num - 1;
}

static bool sl_media_data_jack_cable_is_going_online(struct sl_media_jack *media_jack)
{
	bool is_going_online;

	spin_lock(&media_jack->data_lock);
	is_going_online = media_jack->state == SL_MEDIA_JACK_CABLE_GOING_ONLINE;
	spin_unlock(&media_jack->data_lock);

	return is_going_online;
}

static void sl_media_data_jack_event_online(void *hdl, u8 physical_jack_num)
{
	int                   rtn;
	struct sl_media_jack *media_jack;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "online event (jack_num = %u)", physical_jack_num);

	rtn = sl_media_data_jack_online(hdl, 0, jack_num);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "jack online failed (jack_num = %u) [%d]",
				physical_jack_num, rtn);
		sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
	}
}

static void sl_media_data_jack_event_insert(void *hdl, u8 physical_jack_num)
{
	struct sl_media_jack *media_jack;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "insert event (jack_num = %u)", physical_jack_num);

	media_jack->hdl = hdl;
	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_INSERTED);
}

static void sl_media_data_jack_event_remove(u8 physical_jack_num)
{
	struct sl_media_jack *media_jack;
	u8                    i;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "remove event (jack_num = %u)", physical_jack_num);

	for (i = 0; i < media_jack->port_count; ++i) {
		sl_media_data_jack_media_attr_clr(media_jack, &media_jack->cable_info[i]);
		sl_media_data_jack_cable_high_temp_notif_sent_set(media_jack, &media_jack->cable_info[i], false);
	}

	spin_lock(&media_jack->data_lock);
	sl_media_data_cable_serdes_settings_clr(media_jack);
	sl_media_data_jack_eeprom_clr(media_jack);
	media_jack->temperature_value = -1;
	media_jack->temperature_threshold = -1;
	media_jack->state = SL_MEDIA_JACK_CABLE_REMOVED;
	spin_unlock(&media_jack->data_lock);
	sl_media_data_jack_led_set(media_jack);
	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_NONE);
}

static void sl_media_data_jack_event_offline(u8 physical_jack_num)
{
	struct sl_media_jack *media_jack;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);
	sl_media_log_dbg(media_jack, LOG_NAME, "offline event (jack_num = %u)", physical_jack_num);
	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_OFFLINE);
}

static bool sl_media_data_jack_remove_verify(struct sl_media_jack *media_jack)
{
	u8            i;
	bool          is_removed;

	is_removed = false;
	spin_lock(&media_jack->data_lock);
	if ((!(media_jack->status & XCVR_PRESENT)) &&
		media_jack->state != SL_MEDIA_JACK_CABLE_REMOVED) {
		for (i = 0; i < media_jack->port_count; ++i)
			sl_media_data_jack_media_attr_clr(media_jack, &media_jack->cable_info[i]);
		sl_media_data_cable_serdes_settings_clr(media_jack);
		sl_media_data_jack_eeprom_clr(media_jack);
		media_jack->temperature_value = -1;
		media_jack->temperature_threshold = -1;
		media_jack->state = SL_MEDIA_JACK_CABLE_REMOVED;
		is_removed = true;
	}
	spin_unlock(&media_jack->data_lock);
	sl_media_data_jack_led_set(media_jack);

	return is_removed;
}

static void sl_media_data_jack_online_verify(struct sl_media_jack *media_jack, u8 ldev_num)
{
	int           rtn;
	bool          is_state_different;

	spin_lock(&media_jack->data_lock);
	is_state_different = (media_jack->status & XCVR_PRESENT) &&
			(media_jack->state != SL_MEDIA_JACK_CABLE_ONLINE);
	spin_unlock(&media_jack->data_lock);

	if (is_state_different) {
		rtn = sl_media_data_jack_online(media_jack->hdl, ldev_num, media_jack->num);
		if (rtn) {
			sl_media_log_dbg(media_jack, LOG_NAME, "jack online failed (jack_num = %u) [%d]",
					media_jack->num, rtn);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
		}
	}
}

static int sl_media_data_jack_cable_event(struct notifier_block *event_notifier,
					  unsigned long events, void *data)
{
	int                    rtn;
	void                  *hdl;
	struct xcvr_jack_data  jack_data;
	u8                     physical_jack_num;

	hdl = data;
	rtn = hsnxcvr_jack_get(hdl, &jack_data);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "cable event jack_get failed [%d]", rtn);
		return NOTIFY_OK;
	}

	rtn = kstrtou8(jack_data.name + 1, 10, &physical_jack_num);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "cable event kstrtou8 failed [%d]", rtn);
		return NOTIFY_OK;
	}

	sl_media_log_dbg(NULL, LOG_NAME,
		"cable event (events = 0x%08lX, physical_jack_num = %u)",
		events, physical_jack_num);

	/*
	 * FIXME: Currently servicing one event at a time as we erroneously
	 * get multiple events from hsnxcvr driver. In the future,
	 * we should be servicing all events we get from hsnxcvr driver
	 */
	if (events & HSNXCVR_EVENT_ONLINE)
		sl_media_data_jack_event_online(hdl, physical_jack_num);
	else if (events & HSNXCVR_EVENT_INSERT)
		sl_media_data_jack_event_insert(hdl, physical_jack_num);
	else if (events & HSNXCVR_EVENT_REMOVE)
		sl_media_data_jack_event_remove(physical_jack_num);
	else if (events & HSNXCVR_EVENT_OFFLINE)
		sl_media_data_jack_event_offline(physical_jack_num);

	return NOTIFY_OK;
}

static struct notifier_block event_notifier = {
	.notifier_call = sl_media_data_jack_cable_event,
	.priority = 0,
};

static int sl_media_jack_cable_attr_set(struct sl_media_jack *media_jack, u8 ldev_num,
					struct sl_media_attr *media_attr)
{
	u8  i;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable attr set");

	for (i = 0; i < media_jack->port_count; ++i) {
		media_jack->cable_info[i].ldev_num = ldev_num;
		media_jack->cable_info[i].lgrp_num = media_jack->asic_port[i];
		rtn = sl_media_data_jack_media_attr_set(media_jack, &media_jack->cable_info[i], media_attr);
		if (rtn) {
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_MEDIA_ATTR_SET);
			sl_media_log_err_trace(media_jack, LOG_NAME, "media_attr set failed [%d]", rtn);
			return rtn;
		}
	}

	return 0;
}

int sl_media_data_jack_scan(u8 ldev_num)
{
	u8                       jack_num;
	u8                       physical_jack_num;
	struct sl_media_jack    *media_jack;
	struct sl_media_attr     media_attr;
	void                    *hdl;
	int                      rtn;
	bool                     is_removed;
	struct xcvr_jack_data    jack_data;
	struct xcvr_status_data  status_data;

	sl_media_log_dbg(NULL, LOG_NAME, "jack scan (max_jack_num = %u)", SL_MEDIA_MAX_JACK_NUM);

	memset(&media_attr, 0, sizeof(struct sl_media_attr));
	hdl = NULL;
	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_data_jack_get(ldev_num, jack_num);

		hdl = hsnxcvr_get_next_hdl(hdl);
		if (!hdl) {
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SCAN_HDL_GET);
			sl_media_log_err_trace(NULL, LOG_NAME,
				"jack scan hdl not found (jack_num = %u)", jack_num);
			return 0;
		}
		media_jack->hdl = hdl;

		rtn = hsnxcvr_jack_get(hdl, &jack_data);
		if (rtn) {
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SCAN_JACK_GET);
			sl_media_log_err_trace(NULL, LOG_NAME,
				"jack scan jack_get failed (jack_num = %u)", jack_num);
			continue;
		}
		media_jack->port_count = jack_data.port_count;
		memcpy(&(media_jack->asic_port), &(jack_data.asic_port), sizeof(jack_data.asic_port));

		rtn = kstrtou8(jack_data.name + 1, 10, &physical_jack_num);
		if (rtn) {
			sl_media_log_err(NULL, LOG_NAME, "jack scan kstrtou8 failed [%d]", rtn);
			return -EFAULT;
		}

		sl_media_log_dbg(NULL, LOG_NAME, "jack scan (jack_num = %u, physical_jack_num = %u)",
				 jack_num, physical_jack_num);

		/*
		 * Don't use media_jack in the log messages before this point as they
		 * don't have real physical numbers yet
		 */
		spin_lock(&media_jack->data_lock);
		media_jack->physical_num = physical_jack_num;
		spin_unlock(&media_jack->data_lock);

		rtn = hsnxcvr_status_get(hdl, &status_data);
		if (rtn) {
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SCAN_STATUS_GET);
			sl_media_log_err(media_jack, LOG_NAME,
				"jack scan status_get failed (jack_num = %u)", jack_num);
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			rtn = sl_media_jack_cable_attr_set(media_jack, 0, &media_attr);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME,
					"jack scan cable_attr_set failed [%d]", rtn);
			continue;
		}
		media_jack->status = status_data.flags;

		if (status_data.flags & XCVR_PRESENT) {
			/*
			 * If the jack fails to come online, we set its state to Error and
			 * continue the scan
			 */
			rtn = sl_media_data_jack_online(hdl, ldev_num, jack_num);
			switch (rtn) {
			case -EAGAIN:
				sl_media_log_dbg(media_jack, LOG_NAME,
					"jack scan jack_online failed expect online event later (physical_jack_num = %u) [%d]",
					media_jack->physical_num, rtn);
				sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
				break;
			case 0:
				break;
			default:
				sl_media_log_err_trace(media_jack, LOG_NAME,
					"jack scan jack_online failed (physical_jack_num = %u) [%d]",
					media_jack->physical_num, rtn);
				sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			}
		} else {
			sl_media_log_dbg(media_jack, LOG_NAME,
					 "jack scan jack not present (physical_jack_num = %u)", physical_jack_num);
		}
	}

	rtn = register_hsnxcvr_notifier(&event_notifier);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "jack scan register jack event notifier failed [%d]", rtn);
		return 0;
	}

	/*
	 * Making sure we did not miss any remove or
	 * online event during the window between
	 * first scan and notification registration
	 */
	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_data_jack_get(ldev_num, jack_num);
		rtn = hsnxcvr_status_get(media_jack->hdl, &status_data);
		if (rtn) {
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SCAN_STATUS_GET);
			sl_media_log_err(media_jack, LOG_NAME,
				"jack scan status_get failed (physical_jack_num = %u)",
				media_jack->physical_num);
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			rtn = sl_media_jack_cable_attr_set(media_jack, 0, &media_attr);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME,
					"jack scan cable_attr_set failed [%d]", rtn);
			continue;
		}
		media_jack->status = status_data.flags;

		is_removed = sl_media_data_jack_remove_verify(media_jack);
		if (is_removed)
			continue;

		sl_media_data_jack_online_verify(media_jack, ldev_num);
	}

	sl_media_log_dbg(NULL, LOG_NAME, "jack scan done");

	return 0;
}

void sl_media_data_jack_unregister_event_notifier(void)
{
	unregister_hsnxcvr_notifier(&event_notifier);
}

#define TEMPERATURE_CELSIUS_MIN 10
#define TEMPERATURE_CELSIUS_MAX 200
static int sl_media_data_jack_temp_value_get(struct sl_media_jack *media_jack, u8 *data)
{
	u8  i;
	u8  value;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "temp value get");

	for (i = 0; i < 3; ++i) {
		rtn = sl_media_io_read8(media_jack, 0, 14, &value);
		if (rtn)
			continue;

		if (value < TEMPERATURE_CELSIUS_MIN || value > TEMPERATURE_CELSIUS_MAX)
			continue;

		*data = value;
		return 0;
	}

	return -EINVAL;
}

#define TEMPERATURE_THRESHOLD_CELSIUS_MIN     70
#define TEMPERATURE_THRESHOLD_CELSIUS_MAX     90
#define TEMPERATURE_THRESHOLD_CELSIUS_DEFAULT 80
static int sl_media_data_jack_temp_threshold_get(struct sl_media_jack *media_jack, u8 *data)
{
	u8  i;
	u8  value;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "temp threshold get");

	for (i = 0; i < 3; ++i) {
		rtn = sl_media_io_read8(media_jack, 2, 128, &value);
		if (rtn)
			continue;

		if (value < TEMPERATURE_THRESHOLD_CELSIUS_MIN || value > TEMPERATURE_THRESHOLD_CELSIUS_MAX)
			continue;

		*data = value;
		return 0;
	}

	return -EINVAL;
}

#define SL_MEDIA_JACK_CABLE_EVENT_TIMEOUT 200
int sl_media_data_jack_online(void *hdl, u8 ldev_num, u8 jack_num)
{
	int                     rtn;
	int                     ret;
	u32                     flags;
	struct sl_media_jack   *media_jack;
	struct sl_media_attr    media_attr;
	u8                      count;
	u8                      value;
	struct xcvr_i2c_data    i2c_data;
	struct xcvr_jack_data   jack_data;
	struct xcvr_status_data status_data;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "jack online");

	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_NONE);

	count = 0;
	while (sl_media_data_jack_cable_is_going_online(media_jack)) {
		if (sl_media_jack_state_get(media_jack) == SL_MEDIA_JACK_CABLE_REMOVED) {
			sl_media_log_dbg(media_jack, LOG_NAME,
				"cable removed while waiting for online");
			return 0;
		}
		if (count++ >= SL_MEDIA_JACK_CABLE_EVENT_TIMEOUT) {
			sl_media_jack_fault_cause_set(media_jack,
				SL_MEDIA_FAULT_CAUSE_ONLINE_TIMEDOUT);
			sl_media_log_err(media_jack, LOG_NAME, "timed out waiting for online");
			return -ETIMEDOUT;
		}
		msleep(20);
	}

	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_GOING_ONLINE);

	media_jack->hdl = hdl;

	memset(&media_attr, 0, sizeof(struct sl_media_attr));
	media_attr.magic    = SL_MEDIA_ATTR_MAGIC;
	media_attr.ver      = SL_MEDIA_ATTR_VER;
	media_attr.size     = sizeof(struct sl_media_attr);
	media_attr.errors   = 0;
	media_attr.info     = 0;

	rtn = hsnxcvr_jack_get(media_jack->hdl, &jack_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_GET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "jack get failed [%d]", rtn);
		media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
		ret = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
		if (ret)
			sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", ret);
		return rtn;
	}
	sl_media_log_dbg(media_jack, LOG_NAME,
			 "jack online (jack_type = %u, port_count = %u, asic_port1 = %u, asic_port2 = %u)",
			 jack_data.jack_type, jack_data.port_count, jack_data.asic_port[0], jack_data.asic_port[1]);
	media_jack->port_count = jack_data.port_count;
	memcpy(&(media_jack->asic_port), &(jack_data.asic_port), sizeof(jack_data.asic_port));

	rtn = hsnxcvr_status_get(media_jack->hdl, &status_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_ONLINE_STATUS_GET);
		sl_media_log_err(media_jack, LOG_NAME, "status get failed [%d]", rtn);
		media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
		ret = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
		if (ret)
			sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", ret);
		return rtn;
	}
	media_jack->status = status_data.flags;

	switch (jack_data.jack_type) {
	case XCVR_JACK_BACKPLANE:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_BACKPLANE;
		break;
	case XCVR_JACK_SFP:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_SFP;
		break;
	case XCVR_JACK_QSFP:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_QSFP;
		media_attr.jack_type_info.qsfp.density = SL_MEDIA_QSFP_DENSITY_SINGLE;
		break;
	case XCVR_JACK_QSFPDD:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_QSFP;
		media_attr.jack_type_info.qsfp.density = SL_MEDIA_QSFP_DENSITY_DOUBLE;
		break;
	case XCVR_JACK_OSFP:
	case XCVR_JACK_OSFPXD:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_OSFP;
		break;
	default:
		media_attr.jack_type = SL_MEDIA_JACK_TYPE_INVALID;
	}

	if (media_attr.jack_type != SL_MEDIA_JACK_TYPE_BACKPLANE) {
		i2c_data.addr   = 0;
		i2c_data.page   = 0;
		i2c_data.bank   = 0;
		i2c_data.offset = 0;
		i2c_data.len    = 256;

		rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
		switch (rtn) {
		case -EAGAIN:
			sl_media_log_dbg(media_jack, LOG_NAME, "i2c read failed - expect online event later [%d]", rtn);
			return rtn;
		case 0:
			break;
		default:
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_IO);
			sl_media_log_err_trace(media_jack, LOG_NAME, "i2c read failed [%d]", rtn);
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			ret = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
			if (ret)
				sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", ret);
			return rtn;
		}

		memcpy(media_jack->eeprom_page0, i2c_data.data, SL_MEDIA_EEPROM_PAGE_SIZE);

		rtn = sl_media_data_jack_eeprom_page1_get(media_jack);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "eeprom page1 get failed [%d]", rtn);
			return rtn;
		}

		rtn = sl_media_eeprom_format_get(media_jack, &(media_attr.format));
		if (rtn) {
			memset(&media_attr, 0, sizeof(struct sl_media_attr));
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_FORMAT_INVALID;
			media_attr.errors |= SL_MEDIA_ERROR_TRYABLE;
			ret = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
			if (ret)
				sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", ret);
			return -EFAULT;
		}

		sl_media_eeprom_parse(media_jack, &media_attr);

		if (media_attr.type == SL_MEDIA_TYPE_PEC)
			media_attr.info |= SL_MEDIA_INFO_AUTONEG;

		rtn = sl_media_data_cable_db_ops_cable_validate(&media_attr, media_jack);
		if (rtn) {
			sl_media_log_warn(media_jack, LOG_NAME,
				"cable validate failed [%d] (vendor = %d %s, type = 0x%X %s, length_cm = %d, speeds_map = 0x%lX)",
				rtn, media_attr.vendor, sl_media_vendor_str(media_attr.vendor),
				media_attr.type, sl_media_type_str(media_attr.type),
				media_attr.length_cm, media_attr.speeds_map);
			media_jack->is_cable_not_supported = true;
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_NOT_SUPPORTED;
			media_attr.errors |= SL_MEDIA_ERROR_TRYABLE;
		}

		if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type) &&
		    !media_jack->is_cable_not_supported && !media_jack->is_supported_ss200_cable) {
			if (!sl_media_eeprom_is_fw_version_valid(media_jack, &media_attr)) {
				sl_media_log_warn_trace(media_jack, LOG_NAME, "eeprom fw version invalid");
				media_attr.errors |= SL_MEDIA_ERROR_CABLE_FW_INVALID;
				media_attr.errors |= SL_MEDIA_ERROR_TRYABLE;
			}
			/*
			 * disallow BJ100 speed on active cables
			 */
			media_attr.speeds_map &= ~SL_MEDIA_SPEEDS_SUPPORT_BJ_100G;
		}

		if (media_jack->is_supported_ss200_cable) {
			media_attr.speeds_map  = 0;
			media_attr.speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
			media_attr.speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_100G;
			media_attr.speeds_map |= SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
			media_attr.info       |= SL_MEDIA_INFO_SUPPORTED_SS200_CABLE;
		}
	} else {
		media_attr.vendor        = SL_MEDIA_VENDOR_HPE;
		media_attr.type          = SL_MEDIA_TYPE_BKP;
		media_attr.info          = SL_MEDIA_INFO_AUTONEG;
		media_attr.length_cm     = 25;
		media_attr.hpe_pn        = 60821555;
		media_attr.furcation     = SL_MEDIA_FURCATION_X1;
		media_attr.speeds_map    = SL_MEDIA_SPEEDS_SUPPORT_CK_400G |
					   SL_MEDIA_SPEEDS_SUPPORT_CK_200G |
					   SL_MEDIA_SPEEDS_SUPPORT_BS_200G |
					   SL_MEDIA_SPEEDS_SUPPORT_CK_100G |
					   SL_MEDIA_SPEEDS_SUPPORT_CD_100G |
					   SL_MEDIA_SPEEDS_SUPPORT_BJ_100G |
					   SL_MEDIA_SPEEDS_SUPPORT_CD_50G;
		media_attr.max_speed     = SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
		strncpy(media_attr.serial_num_str, "AK20212120", sizeof(media_attr.serial_num_str));
		strncpy(media_attr.hpe_pn_str, "PK60821-555", sizeof(media_attr.hpe_pn_str));
		strncpy(media_attr.date_code_str, "08-19-21", sizeof(media_attr.date_code_str));
		memset(media_attr.fw_ver, 0, sizeof(media_attr.fw_ver));
	}

	flags = 0;
	if (media_attr.vendor == SL_MEDIA_VENDOR_MULTILANE)
		flags |= SL_MEDIA_TYPE_LOOPBACK;
	if (media_attr.jack_type == SL_MEDIA_JACK_TYPE_BACKPLANE)
		flags |= SL_MEDIA_TYPE_BACKPLANE;
	if (media_jack->is_cable_not_supported)
		flags |= SL_MEDIA_TYPE_NOT_SUPPORTED;

	rtn = sl_media_data_cable_db_ops_serdes_settings_get(media_jack, media_attr.type, flags);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SERDES_SETTINGS_GET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "serdes settings get failed [%d]", rtn);
		ret = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
		if (ret)
			sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", ret);
		return rtn;
	}

	if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type)) {
		rtn = sl_media_data_jack_cable_soft_reset(media_jack);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "cable soft reset failed [%d]", rtn);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			ret = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
			if (ret)
				sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", ret);
			return rtn;
		}

		rtn = sl_media_jack_cable_high_power_set(ldev_num, jack_num);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "high power set failed [%d]", rtn);
			sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
			media_attr.errors |= SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT;
			ret = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
			if (ret)
				sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", ret);
			return rtn;
		}
	}

	if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type)) {
		rtn = sl_media_data_jack_temp_threshold_get(media_jack, &value);
		if (rtn) {
			media_jack->temperature_threshold = TEMPERATURE_THRESHOLD_CELSIUS_DEFAULT;
			media_attr.errors |= SL_MEDIA_ERROR_TEMP_THRESH_DEFAULT;
			sl_media_log_dbg(media_jack, LOG_NAME, "media error cable temp theshold (0x%x)",
					 media_attr.errors);
		} else {
			media_jack->temperature_threshold = value;
		}
	}

	rtn = sl_media_jack_cable_attr_set(media_jack, ldev_num, &media_attr);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set failed [%d]", rtn);
		return rtn;
	}

	if (SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_attr.type)) {
		if (sl_media_data_jack_cable_hw_shift_state_get(media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED)
			sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_DOWNSHIFTED);
		else if (sl_media_data_jack_cable_hw_shift_state_get(media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED)
			sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_UPSHIFTED);
		else
			sl_media_jack_cable_shift_state_set(media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_INVALID);
	}

	spin_lock(&media_jack->data_lock);
	media_jack->is_high_temp = false;
	spin_unlock(&media_jack->data_lock);

	sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ONLINE);

	sl_media_log_dbg(media_jack, LOG_NAME, "online (jack = 0x%p)", media_jack);

	return 0;
}

int sl_media_data_jack_lgrp_connect(struct sl_media_lgrp *media_lgrp)
{
	struct sl_media_jack *media_jack;
	u8                    jack_num;
	u8                    i;

	sl_media_log_dbg(media_lgrp, LOG_NAME,
			 "jack lgrp connect (max_jack_num = %u)", SL_MEDIA_MAX_JACK_NUM);

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_data_jack_get(media_lgrp->media_ldev->num, jack_num);
		for (i = 0; i < media_jack->port_count; ++i) {
			sl_media_log_dbg(media_lgrp, LOG_NAME,
					 "jack lgrp connect (jack_num = %u, asic_port%u = %u)",
					 jack_num, i, media_jack->asic_port[i]);
			if (media_jack->asic_port[i] == media_lgrp->num) {
				sl_media_log_dbg(media_lgrp, LOG_NAME, "jack lgrp connect found");
				media_lgrp->media_jack = media_jack;
				media_lgrp->cable_info = &media_jack->cable_info[i];
				return 0;
			}
		}
	}

	sl_media_log_dbg(media_lgrp, LOG_NAME, "jack lgrp connect not found");
	return -EFAULT;
}

#define DATA_PATH_STATE_DEACTIVATED       0x1
#define DATA_PATH_EXPLICIT_CONTROL_ENABLE 0x00
#define DATA_PATH_ID                      0x08
#define DATA_PATH_LOWER_LANE_CONFIG       (DATA_PATH_EXPLICIT_CONTROL_ENABLE)
#define DATA_PATH_UPPER_LANE_CONFIG       (DATA_PATH_EXPLICIT_CONTROL_ENABLE | DATA_PATH_ID)
#define DATA_PATH_LANES_DEACTIVATED       (DATA_PATH_STATE_DEACTIVATED << 4 | DATA_PATH_STATE_DEACTIVATED)
int sl_media_data_jack_cable_downshift(struct sl_media_jack *media_jack)
{
	int                  rtn;
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable downshift");

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_BUSY);

	/*
	 * Deinit all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/*
	 * enable low power mode
	 */
	rtn = sl_media_data_jack_cable_low_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "low power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/*
	 * Staged Control Set 0, Data Path Configuration bytes @ page 0x10 bytes 145-152
	 * Config lanes 1 to 4
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 145;
	i2c_data.data[0] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 1-4 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/*
	 * Config lanes 5 to 8
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 149;
	i2c_data.data[0] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 5-8 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/*
	 * ApplyDPInitLane8-1
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 143;
	i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "apply dpinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/*
	 * enable high power mode
	 */
	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(8000);

	/*
	 * (Re)Init all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	i2c_data.data[0] = 0x00;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0x00 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}

	/*
	 * waiting for firmware reload
	 */
	msleep(3000);

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);

	return 0;
}

int sl_media_data_jack_cable_hw_shift_state_get(struct sl_media_jack *media_jack)
{
	int                  rtn;
	u8                   downshift_lower_lane_config;
	u8                   downshift_upper_lane_config;
	u8                   upshift_lower_lane_config;
	u8                   upshift_upper_lane_config;
	u8                   read_data_lower[4];
	u8                   read_data_upper[4];
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable hw shift state get");

	downshift_lower_lane_config = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	downshift_upper_lane_config = (media_jack->appsel_num_200_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;

	upshift_lower_lane_config = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	upshift_upper_lane_config = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;

	i2c_data.addr   = 0;
	i2c_data.page   = 0x10;
	i2c_data.bank   = 0;
	i2c_data.offset = 145;
	i2c_data.len    = 4;
	rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_STATE_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				 "SCS0 configuration - config lanes 1-4 - read failed [%d]", rtn);
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_INVALID;
	}
	memcpy(read_data_lower, i2c_data.data, sizeof(read_data_lower));

	i2c_data.addr   = 0;
	i2c_data.page   = 0x10;
	i2c_data.bank   = 0;
	i2c_data.offset = 149;
	i2c_data.len    = 4;
	rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_STATE_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				 "SCS0 configuration - config lanes 5-8 - read failed [%d]", rtn);
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_INVALID;
	}
	memcpy(read_data_upper, i2c_data.data, sizeof(read_data_upper));

	if ((read_data_lower[0] == downshift_lower_lane_config) &&
		(read_data_lower[1] == downshift_lower_lane_config) &&
		(read_data_lower[2] == downshift_lower_lane_config) &&
		(read_data_lower[3] == downshift_lower_lane_config) &&
		(read_data_upper[0] == downshift_upper_lane_config) &&
		(read_data_upper[1] == downshift_upper_lane_config) &&
		(read_data_upper[2] == downshift_upper_lane_config) &&
		(read_data_upper[3] == downshift_upper_lane_config)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable is downshifted");
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED;
	} else if ((read_data_lower[0] == upshift_lower_lane_config) &&
		(read_data_lower[1] == upshift_lower_lane_config) &&
		(read_data_lower[2] == upshift_lower_lane_config) &&
		(read_data_lower[3] == upshift_lower_lane_config) &&
		(read_data_upper[0] == upshift_upper_lane_config) &&
		(read_data_upper[1] == upshift_upper_lane_config) &&
		(read_data_upper[2] == upshift_upper_lane_config) &&
		(read_data_upper[3] == upshift_upper_lane_config)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable is upshifted");
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable is in invalid state");
	return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_INVALID;
}

int sl_media_data_jack_cable_upshift(struct sl_media_jack *media_jack)
{
	int                  rtn;
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable upshift");

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_BUSY);

	/*
	 * Deinit all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/*
	 * enable low power mode
	 */
	rtn = sl_media_data_jack_cable_low_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "low power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/*
	 * Staged Control Set 0, Data Path Configuration bytes @ page 0x10 bytes 145-152
	 * Config lanes 1 to 4
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 145;
	i2c_data.data[0] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 1-4 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/*
	 * Config lanes 5 to 8
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 149;
	i2c_data.data[0] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_400_gaui << 4) | DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 5-8 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/*
	 * ApplyDPInitLane8-1
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 143;
	i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "apply dpinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/*
	 * enable high power mode
	 */
	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(8000);

	/*
	 * (Re)Init all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	i2c_data.data[0] = 0x00;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0x00 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}

	/*
	 * waiting for firmware reload
	 */
	msleep(3000);

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);

	return 0;
}

int sl_media_data_jack_cable_soft_reset(struct sl_media_jack *media_jack)
{
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable soft reset");

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_BUSY);

	i2c_data.page    = 0;
	i2c_data.bank    = 0;
	i2c_data.offset  = 0x1a;
	i2c_data.data[0] = 0x08;
	i2c_data.len     = 1;
	hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);

	msleep(300);

	i2c_data.page    = 0;
	i2c_data.bank    = 0;
	i2c_data.offset  = 0x1a;
	i2c_data.data[0] = 0x00;
	i2c_data.len     = 1;
	hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);

	/*
	 * waiting for firmware reload
	 */
	msleep(3000);

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);

	return 0;
}

#define SL_MEDIA_CMIS_POWER_UP_PAGE 0x00
#define SL_MEDIA_CMIS_POWER_UP_ADDR 0x1a
#define SL_MEDIA_CMIS_POWER_UP_DATA 0x00
int sl_media_data_jack_cable_high_power_set(struct sl_media_jack *media_jack)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "high power set");

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_CMIS_POWER_UP_PAGE,
	      SL_MEDIA_CMIS_POWER_UP_ADDR, SL_MEDIA_CMIS_POWER_UP_DATA);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = true;

	return 0;
}

#define SL_MEDIA_CMIS_POWER_DOWN_PAGE 0x00
#define SL_MEDIA_CMIS_POWER_DOWN_ADDR 0x1a
#define SL_MEDIA_CMIS_POWER_DOWN_DATA 0x10
int sl_media_data_jack_cable_low_power_set(struct sl_media_jack *media_jack)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "low power set");

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_CMIS_POWER_DOWN_PAGE,
	      SL_MEDIA_CMIS_POWER_DOWN_ADDR, SL_MEDIA_CMIS_POWER_DOWN_DATA);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = false;

	return 0;
}

#define LED_OFF        XCVR_LED_OFF
#define LED_ON_GRN     XCVR_LED_A_STEADY
#define LED_FAST_GRN   XCVR_LED_A_FAST
#define LED_ON_AMBER   XCVR_LED_B_STEADY
#define LED_FAST_AMBER XCVR_LED_B_FAST
void sl_media_data_jack_led_set(struct sl_media_jack *media_jack)
{
	int                   rtn;
	struct sl_core_link  *core_link;
	struct sl_core_lgrp  *core_lgrp;
	u32                   link_state;
	bool                  link_going_up;
	bool                  link_up;
	u8                    link_num;
	u8                    ldev_num;
	u8                    lgrp_num;
	u8                    max_links;
	u8                    i;

	sl_media_log_dbg(media_jack, LOG_NAME, "led set");

	if (sl_media_data_jack_cable_is_high_temp(media_jack)) {
		sl_media_io_led_set(media_jack, LED_ON_AMBER);
		return;
	}

	switch (sl_media_jack_state_get(media_jack)) {
	case SL_MEDIA_JACK_CABLE_REMOVED:
		sl_media_io_led_set(media_jack, LED_OFF);
		return;
	case SL_MEDIA_JACK_CABLE_ERROR:
		sl_media_io_led_set(media_jack, LED_FAST_AMBER);
		return;
	}

	link_going_up = false;
	link_up       = false;
	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;

		core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
		if (!core_lgrp)
			continue;

		switch (core_lgrp->config.furcation) {
		case SL_MEDIA_FURCATION_X1:
			max_links = 1;
			break;
		case SL_MEDIA_FURCATION_X2:
			max_links = 2;
			break;
		case SL_MEDIA_FURCATION_X4:
			max_links = 4;
			break;
		default:
			max_links = 0;
		}

		for (link_num = 0; link_num < max_links; ++link_num) {
			core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
			if (!core_link)
				continue;

			rtn = sl_core_link_state_get(ldev_num, lgrp_num, link_num, &link_state);
			if (rtn) {
				sl_media_log_err_trace(media_jack, LOG_NAME,
						       "link state get failed (ldev=%u, lgrp=%u, link=%u) [%d] ",
						       ldev_num, lgrp_num, link_num, rtn);
				return;
			}

			switch (link_state) {
			case SL_CORE_LINK_STATE_GOING_UP:
			case SL_CORE_LINK_STATE_AN:
				link_going_up = true;
				break;
			case SL_CORE_LINK_STATE_UP:
				link_up = true;
				break;
			default:
				break;
			}
		}
	}

	if (link_going_up)
		sl_media_io_led_set(media_jack, LED_FAST_GRN);
	else if (link_up)
		sl_media_io_led_set(media_jack, LED_ON_GRN);
	else
		sl_media_io_led_set(media_jack, LED_OFF);
}

static int sl_media_data_jack_cable_high_temp_link_down(struct sl_media_jack *media_jack)
{
	int                  rtn;
	int                  i;
	struct sl_ctrl_link *ctrl_link;
	u8                   ldev_num;
	u8                   lgrp_num;
	u8                   link_num;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable high temp link down");

	sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_HIGH_TEMP);

	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;

		if (sl_media_data_jack_cable_is_high_temp_client_ready(media_jack, &media_jack->cable_info[i]) &&
		    !sl_media_data_jack_cable_is_high_temp_notif_sent(media_jack, &media_jack->cable_info[i])) {
			rtn = sl_ctrl_lgrp_notif_enqueue(sl_ctrl_lgrp_get(ldev_num, lgrp_num),
							 SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_HIGH_TEMP, NULL, 0);
			if (rtn)
				sl_media_log_warn_trace(media_jack, LOG_NAME,
							"cable high temp notif send enqueue failed [%d]",
							rtn);
			else
				sl_media_data_jack_cable_high_temp_notif_sent_set(media_jack,
										  &media_jack->cable_info[i],
										  true);
		}

		for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
			ctrl_link = sl_ctrl_link_get(ldev_num, lgrp_num, link_num);
			if (!ctrl_link)
				continue;

			rtn = sl_ctrl_link_async_down(ctrl_link, SL_LINK_DOWN_CAUSE_HIGH_TEMP_FAULT_MAP);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME,
						       "cable high temp link down async_down failed [%d]", rtn);
		}
	}
	return 0;
}

static void sl_media_data_jack_cable_monitor_high_temp_delayed_work(struct work_struct *work)
{
	int                   rtn;
	u8                    jack_num;
	struct sl_media_jack *media_jack;
	struct sl_media_ldev *media_ldev;
	struct delayed_work  *delayed_work_ptr;

	delayed_work_ptr  = container_of(work, struct delayed_work, work);
	media_ldev = container_of(delayed_work_ptr, struct sl_media_ldev,
				  delayed_work[SL_MEDIA_WORK_CABLE_MON_HIGH_TEMP]);

	sl_media_log_dbg(media_ldev, LOG_NAME, "cable monitor high temp work (ldev = 0x%p)", media_ldev);

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_data_jack_get(media_ldev->num, jack_num);
		if (!media_jack)
			continue;

		if (sl_media_data_jack_is_headshell_busy(media_jack))
			continue;

		if (!sl_media_lgrp_get(media_jack->cable_info[0].ldev_num, media_jack->cable_info[0].lgrp_num))
			continue;

		if (!SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_jack->cable_info[0].media_attr.type)) {
			sl_media_log_dbg(media_ldev, LOG_NAME, "not active cable (jack_num = %u)", jack_num);
			continue;
		}

		if (!sl_media_data_jack_cable_is_high_temp_set(media_jack))
			continue;

		rtn = sl_media_data_jack_cable_high_temp_link_down(media_jack);
		if (rtn)
			sl_media_log_err_trace(media_ldev, LOG_NAME,
					       "cable monitor high temp link_down is failed [%d]", rtn);
	}

	queue_delayed_work(media_ldev->workqueue, &media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_HIGH_TEMP],
			   msecs_to_jiffies(SL_MEDIA_HIGH_TEMP_MONITOR_TIME_MS));
}

bool sl_media_data_jack_cable_is_high_temp_client_ready(struct sl_media_jack *media_jack,
							struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_client_ready;

	spin_lock(&media_jack->data_lock);
	is_client_ready = cable_info->high_temp_client_ready;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "cable is high temp client ready (%s)", is_client_ready ? "yes" : "no");

	return is_client_ready;
}

#define TEMPERATURE_CELSIUS_SLOPE 10
bool sl_media_data_jack_cable_is_high_temp_set(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  current_temp;
	int prev_temp;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable is high temp set");

	/* If "is_high_temp" flag set, it indicates high temperature was detected before and the action
	 * has been taken. If the notification was not sent previously, it re-tries to send it.
	 * Returning false in this case short-circuits the flow, signaling the caller that no further
	 * action is required.
	 */
	spin_lock(&media_jack->data_lock);
	if (media_jack->is_high_temp) {
		spin_unlock(&media_jack->data_lock);
		sl_media_data_jack_cable_high_temp_notif_send(media_jack);
		return false;
	}
	spin_unlock(&media_jack->data_lock);

	rtn = sl_media_data_jack_temp_value_get(media_jack, &current_temp);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "temperature value read failed [%d]", rtn);
		media_jack->temperature_value = -1;
		return false;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "cable is high temp set (temperature = 0x%X, threshold = 0x%X)",
			 current_temp, media_jack->temperature_threshold);

	prev_temp = media_jack->temperature_value;
	media_jack->temperature_value = current_temp;

	if (prev_temp < 0)
		goto out;

	if (current_temp < prev_temp - TEMPERATURE_CELSIUS_SLOPE ||
	    current_temp > prev_temp + TEMPERATURE_CELSIUS_SLOPE) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "temperature slope check failure (temperature = %dc, previous = %dc, slope = %dc)",
				       current_temp, prev_temp, TEMPERATURE_CELSIUS_SLOPE);
		return false;
	}

out:
	if (current_temp > media_jack->temperature_threshold) {
		spin_lock(&media_jack->data_lock);
		media_jack->is_high_temp = true;
		sl_media_log_warn(media_jack, LOG_NAME,
				  "cable high temperature alert (temperature = %dc, threshold = %dc)",
				  current_temp, media_jack->temperature_threshold);
		spin_unlock(&media_jack->data_lock);
		return true;
	}

	return false;
}

void sl_media_data_jack_cable_is_high_temp_clr(struct sl_media_jack *media_jack)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cable is high temp clr");

	spin_lock(&media_jack->data_lock);
	media_jack->is_high_temp = false;
	spin_unlock(&media_jack->data_lock);
}

bool sl_media_data_jack_cable_is_high_temp_notif_sent(struct sl_media_jack *media_jack,
						      struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_notif_sent;

	spin_lock(&media_jack->data_lock);
	is_notif_sent = cable_info->high_temp_notif_sent;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "cable is high temp notif sent (%s)", is_notif_sent ? "yes" : "no");

	return is_notif_sent;
}

int sl_media_data_jack_cable_temp_get(struct sl_media_jack *media_jack, u8 *temp)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cable temp get");

	if (!sl_media_lgrp_media_type_is_active(media_jack->cable_info[0].ldev_num,
						media_jack->cable_info[0].lgrp_num)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "not active cable");
		return -EBADRQC;
	}

	if (media_jack->temperature_value < 0)
		return -EIO;

	*temp = media_jack->temperature_value;
	return 0;
}

int sl_media_data_jack_cable_high_temp_threshold_get(struct sl_media_jack *media_jack, u8 *temp_threshold)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cable high temp threshold get");

	if (!sl_media_lgrp_media_type_is_active(media_jack->cable_info[0].ldev_num,
						media_jack->cable_info[0].lgrp_num)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "not active cable");
		return -EBADRQC;
	}

	if (media_jack->temperature_threshold < 0)
		return -EIO;

	*temp_threshold = media_jack->temperature_threshold;
	return 0;
}

void sl_media_data_jack_cable_high_temp_notif_send(struct sl_media_jack *media_jack)
{
	u8  i;
	u8  ldev_num;
	u8  lgrp_num;
	int rtn;

	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;
		if (sl_media_data_jack_cable_is_high_temp_client_ready(media_jack, &media_jack->cable_info[i]) &&
		    !sl_media_data_jack_cable_is_high_temp_notif_sent(media_jack, &media_jack->cable_info[i])) {
			rtn = sl_ctrl_lgrp_notif_enqueue(sl_ctrl_lgrp_get(ldev_num, lgrp_num),
							 SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_HIGH_TEMP,
							 NULL, 0);
			if (rtn)
				sl_media_log_warn_trace(media_jack, LOG_NAME,
							"cable high temp notif send enqueue failed [%d]",
							rtn);
			else
				sl_media_data_jack_cable_high_temp_notif_sent_set(media_jack,
										  &media_jack->cable_info[i],
										  true);
		}
	}
}

void sl_media_data_jack_cable_high_temp_notif_sent_set(struct sl_media_jack *media_jack,
						       struct sl_media_lgrp_cable_info *cable_info, bool value)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cable high temp notif sent set (%s)", value ? "true" : "false");

	spin_lock(&media_jack->data_lock);
	cable_info->high_temp_notif_sent = value;
	spin_unlock(&media_jack->data_lock);
}

void sl_media_data_jack_cable_high_temp_monitor_start(struct sl_media_ldev *media_ldev)
{
	INIT_DELAYED_WORK(&media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_HIGH_TEMP],
			  sl_media_data_jack_cable_monitor_high_temp_delayed_work);

	queue_delayed_work(media_ldev->workqueue, &media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_HIGH_TEMP],
			   msecs_to_jiffies(SL_MEDIA_HIGH_TEMP_MONITOR_TIME_MS));
}

void sl_media_data_jack_cable_high_temp_monitor_stop(struct sl_media_ldev *media_ldev)
{
	cancel_delayed_work_sync(&media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_HIGH_TEMP]);
}
