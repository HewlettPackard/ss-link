// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/sl_media.h>
#include <linux/string.h>

#include "sl_asic.h"

#include "base/sl_media_log.h"
#include "base/sl_media_eeprom.h"

#include "uapi/sl_media.h"
#include "sl_media_jack.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_jack_rosetta.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_cable_db_ops.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

#define FLAT_MEM_OFFSET 2
#define FLAT_MEM_BIT    7
static int sl_media_data_jack_eeprom_page1_get(struct sl_media_jack *media_jack)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "eeprom page1 get");

	if ((media_jack->eeprom_page0[FLAT_MEM_OFFSET] & BIT(FLAT_MEM_BIT)) != 0) {
		sl_media_log_dbg(media_jack, LOG_NAME, "no page1 in eeprom");
		return 0;
	}

	media_jack->i2c_data.addr   = 0;
	media_jack->i2c_data.page   = 1;
	media_jack->i2c_data.bank   = 0;
	media_jack->i2c_data.offset = 0;
	media_jack->i2c_data.len    = 256;

	rtn = hsnxcvr_i2c_read(media_jack->hdl, &media_jack->i2c_data);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "i2c read for eeprom page1 failed [%d]", rtn);
		return rtn;
	}
	memcpy(media_jack->eeprom_page1, media_jack->i2c_data.data, SL_MEDIA_EEPROM_PAGE_SIZE);

	return 0;
}

/*
 * To deal with backplane jack numbers
 */
static inline u8 sl_media_data_jack_num_update(u8 jack_num)
{
	if (jack_num >= 100)
		return jack_num - 76;

	return jack_num - 1;
}

static bool sl_media_data_jack_cable_is_going_online(struct sl_media_jack *media_jack)
{
	bool          is_going_online;
	unsigned long irq_flags;

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	is_going_online = media_jack->state == SL_MEDIA_JACK_CABLE_GOING_ONLINE;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	return is_going_online;
}

static void sl_media_data_jack_event_online(void *hdl, u8 physical_jack_num)
{
	int                   rtn;
	struct sl_media_jack *media_jack;
	unsigned long         irq_flags;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "online event (jack_num = %u)", physical_jack_num);

	rtn = sl_media_data_jack_online(hdl, 0, jack_num);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "jack online failed (jack_num = %u) [%d]",
				physical_jack_num, rtn);
		spin_lock_irqsave(&media_jack->data_lock, irq_flags);
		media_jack->state = SL_MEDIA_JACK_CABLE_ERROR;
		spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
	}
}

static void sl_media_data_jack_event_insert(void *hdl, u8 physical_jack_num)
{
	struct sl_media_jack *media_jack;
	unsigned long         irq_flags;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "insert event (jack_num = %u)", physical_jack_num);

	media_jack->hdl = hdl;
	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->state = SL_MEDIA_JACK_CABLE_INSERTED;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
}

static void sl_media_data_jack_event_remove(u8 physical_jack_num)
{
	struct sl_media_jack *media_jack;
	u8                    i;
	unsigned long         irq_flags;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "remove event (jack_num = %u)", physical_jack_num);

	for (i = 0; i < media_jack->jack_data.port_count; ++i)
		sl_media_data_jack_media_attr_clr(media_jack, &media_jack->cable_info[i]);

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	sl_media_data_cable_serdes_settings_clr(media_jack);
	sl_media_data_jack_eeprom_clr(media_jack);
	media_jack->state = SL_MEDIA_JACK_CABLE_REMOVED;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
}

static void sl_media_data_jack_event_offline(u8 physical_jack_num)
{
	struct sl_media_jack *media_jack;
	unsigned long         irq_flags;
	u8                    jack_num;

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);
	sl_media_log_dbg(media_jack, LOG_NAME, "offline event (jack_num = %u)", physical_jack_num);
	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->state = SL_MEDIA_JACK_CABLE_ERROR;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
}

/*
 * Interrupts will only come from active cables
 */
static void sl_media_data_jack_event_interrupt(u8 physical_jack_num)
{
	int                   rtn;
	struct sl_media_jack *media_jack;
	u8                    jack_num;

	sl_media_log_dbg(NULL, LOG_NAME, "interrupt event (jack_num = %u)", physical_jack_num);

	jack_num = sl_media_data_jack_num_update(physical_jack_num);
	media_jack = sl_media_data_jack_get(0, jack_num);

	/*
	 * Reading Flags from CMIS v5
	 */
	media_jack->i2c_data.addr   = 0;
	media_jack->i2c_data.page   = 0;
	media_jack->i2c_data.bank   = 0;
	media_jack->i2c_data.offset = 8;
	media_jack->i2c_data.len    = 4;
	rtn = hsnxcvr_i2c_read(media_jack->hdl, &media_jack->i2c_data);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "i2c read failed [%d]", rtn);
		return;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "Module Level Flags");

	sl_media_log_dbg(media_jack, LOG_NAME, "ModState/ModFW/DPFW/CdbCmdComplete: %u",
				media_jack->i2c_data.data[0]);

	sl_media_log_dbg(media_jack, LOG_NAME, "TempMon/VccMon:                     %u",
				media_jack->i2c_data.data[1]);

	sl_media_log_dbg(media_jack, LOG_NAME, "AuxMon:                             %u",
				media_jack->i2c_data.data[2]);

	sl_media_log_dbg(media_jack, LOG_NAME, "CustomMon:                          %u",
				media_jack->i2c_data.data[3]);

	/*
	 * Reading Flags from CMIS v5
	 */
	media_jack->i2c_data.addr   = 0;
	media_jack->i2c_data.page   = 17;
	media_jack->i2c_data.bank   = 0;
	media_jack->i2c_data.offset = 134;
	media_jack->i2c_data.len    = 20;
	rtn = hsnxcvr_i2c_read(media_jack->hdl, &media_jack->i2c_data);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "i2c read failed [%d]", rtn);
		return;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "Lane Specific Flags");

	sl_media_log_dbg(media_jack, LOG_NAME, "DPState:                      %u",
			media_jack->i2c_data.data[0]);

	sl_media_log_dbg(media_jack, LOG_NAME, "FailureFlagTx:                %u",
			media_jack->i2c_data.data[1]);

	sl_media_log_dbg(media_jack, LOG_NAME, "LosFlagTx:                    %u",
			media_jack->i2c_data.data[2]);

	sl_media_log_dbg(media_jack, LOG_NAME, "CDRLOLFlagTx:                 %u",
			media_jack->i2c_data.data[3]);

	sl_media_log_dbg(media_jack, LOG_NAME, "AdaptiveIpEqFailTX:           %u",
			media_jack->i2c_data.data[4]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerHighAlarmTX:      %u",
			media_jack->i2c_data.data[5]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerLowAlarmTX:       %u",
			media_jack->i2c_data.data[6]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerHighWarningTx:    %u",
			media_jack->i2c_data.data[7]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerLowWarningTx:     %u",
			media_jack->i2c_data.data[8]);

	sl_media_log_dbg(media_jack, LOG_NAME, "LaserBiasHighAlarmTx:         %u",
			media_jack->i2c_data.data[9]);

	sl_media_log_dbg(media_jack, LOG_NAME, "LaserBiasLowAlarmTx:          %u",
			media_jack->i2c_data.data[10]);

	sl_media_log_dbg(media_jack, LOG_NAME, "LaserBiasHighWarningTx:       %u",
			media_jack->i2c_data.data[11]);

	sl_media_log_dbg(media_jack, LOG_NAME, "LaserBiasLowWarningTx:        %u",
			media_jack->i2c_data.data[12]);

	sl_media_log_dbg(media_jack, LOG_NAME, "LosFlagRx:                    %u",
			media_jack->i2c_data.data[13]);

	sl_media_log_dbg(media_jack, LOG_NAME, "CDRLOLFlagRx:                 %u",
			media_jack->i2c_data.data[14]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerHighAlarmRX:      %u",
			media_jack->i2c_data.data[15]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerLowAlarmRX:       %u",
			media_jack->i2c_data.data[16]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerHighWarningRx:    %u",
			media_jack->i2c_data.data[17]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalPowerLowWarningRx:     %u",
			media_jack->i2c_data.data[18]);

	sl_media_log_dbg(media_jack, LOG_NAME, "OpticalStatusChangedRx:       %u",
			media_jack->i2c_data.data[19]);
}

static bool sl_media_data_jack_remove_verify(struct sl_media_jack *media_jack)
{
	unsigned long irq_flags;
	u8            i;
	bool          is_removed;

	is_removed = false;
	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	if ((!(media_jack->status_data.flags & XCVR_PRESENT)) &&
		media_jack->state != SL_MEDIA_JACK_CABLE_REMOVED) {
		for (i = 0; i < media_jack->jack_data.port_count; ++i)
			sl_media_data_jack_media_attr_clr(media_jack, &media_jack->cable_info[i]);
		sl_media_data_cable_serdes_settings_clr(media_jack);
		sl_media_data_jack_eeprom_clr(media_jack);
		media_jack->state = SL_MEDIA_JACK_CABLE_REMOVED;
		is_removed = true;
	}
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	return is_removed;
}

static void sl_media_data_jack_online_verify(struct sl_media_jack *media_jack, u8 ldev_num)
{
	unsigned long irq_flags;
	int           rtn;
	bool          is_state_different;

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	is_state_different = (media_jack->status_data.flags & XCVR_PRESENT) &&
			(media_jack->state != SL_MEDIA_JACK_CABLE_ONLINE);
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	if (is_state_different) {
		rtn = sl_media_data_jack_online(media_jack->hdl, ldev_num, media_jack->num);
		if (rtn) {
			sl_media_log_dbg(media_jack, LOG_NAME, "jack online failed (jack_num = %u) [%d]",
					media_jack->num, rtn);
			spin_lock_irqsave(&media_jack->data_lock, irq_flags);
			media_jack->state = SL_MEDIA_JACK_CABLE_ERROR;
			spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
		}
	}
}

static int sl_media_data_jack_cable_event(struct notifier_block *event_notifier,
			unsigned long events, void *data)
{
	int                      rtn;
	void                    *hdl;
	struct xcvr_jack_data    jack;
	u8                       physical_jack_num;

	hdl = data;
	rtn = hsnxcvr_jack_get(hdl, &jack);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "jack get failed [%d]", rtn);
		return NOTIFY_OK;
	}

	rtn = kstrtou8(jack.name + 1, 10, &physical_jack_num);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "kstrtou8 failed [%d]", rtn);
		return NOTIFY_OK;
	}

	sl_media_log_dbg(NULL, LOG_NAME, "cable event (events = 0x%08lX, jack_num = %u)",
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

	if (events & HSNXCVR_EVENT_INTERRUPT)
		sl_media_data_jack_event_interrupt(physical_jack_num);

	return NOTIFY_OK;
}

static struct notifier_block event_notifier = {
	.notifier_call = sl_media_data_jack_cable_event,
	.priority = 0,
};

int sl_media_data_jack_scan(u8 ldev_num)
{
	u8                       jack_cntr;
	u8                       jack_num;
	u8                       physical_jack_num;
	unsigned long            irq_flags;
	struct xcvr_status_data  status;
	struct xcvr_jack_data    jack;
	struct sl_media_jack    *media_jack;
	void                    *hdl;
	int                      rtn;
	bool                     is_removed;
	u8                       i;

	sl_media_log_dbg(NULL, LOG_NAME, "jack scan");

	hdl = NULL;
	for (jack_cntr = 0; jack_cntr < SL_MEDIA_MAX_JACK_NUM; ++jack_cntr) {
		hdl = hsnxcvr_get_next_hdl(hdl);
		if (!hdl) {
			sl_media_log_err(NULL, LOG_NAME, "hdl not found (jack_num = %u)", jack_cntr);
			return -EFAULT;
		}
		rtn = hsnxcvr_status_get(hdl, &status);
		if (rtn) {
			sl_media_log_err(NULL, LOG_NAME, "status_get failed (jack_num = %u)", jack_cntr);
			return -EFAULT;
		}
		rtn = hsnxcvr_jack_get(hdl, &jack);
		if (rtn) {
			sl_media_log_err(NULL, LOG_NAME, "jack_get failed (jack_num = %u)", jack_cntr);
			return -EFAULT;
		}

		rtn = kstrtou8(jack.name + 1, 10, &physical_jack_num);
		if (rtn) {
			sl_media_log_err(NULL, LOG_NAME, "kstrtou8 failed [%d]", rtn);
			return -EFAULT;
		}
		jack_num = sl_media_data_jack_num_update(physical_jack_num);
		media_jack = sl_media_data_jack_get(ldev_num, jack_num);
		spin_lock_irqsave(&media_jack->data_lock, irq_flags);
		media_jack->physical_num = physical_jack_num;
		spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

		for (i = 0; i < jack.port_count; ++i)
			sl_media_log_dbg(media_jack, LOG_NAME, "jack_name = %s, asic_port = %u",
				jack.name, jack.asic_port[i]);

		if (status.flags & XCVR_PRESENT) {
			/*
			 * If the jack fails to come online, we set its state to Error and
			 * continue the scan
			 */
			rtn = sl_media_data_jack_online(hdl, ldev_num, jack_num);
			switch (rtn) {
			case -EAGAIN:
				sl_media_log_dbg(media_jack, LOG_NAME,
						"jack online failed - expect online event later (jack_num = %u) [%d]",
						media_jack->physical_num, rtn);
				spin_lock_irqsave(&media_jack->data_lock, irq_flags);
				media_jack->state = SL_MEDIA_JACK_CABLE_ERROR;
				spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
				break;
			case 0:
				break;
			default:
				sl_media_log_err(media_jack, LOG_NAME, "jack online failed (jack_num = %u) [%d]",
						media_jack->physical_num, rtn);
				spin_lock_irqsave(&media_jack->data_lock, irq_flags);
				media_jack->state = SL_MEDIA_JACK_CABLE_ERROR;
				spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
			}
		} else {
			media_jack->jack_data = jack;
		}
	}

	rtn = register_hsnxcvr_notifier(&event_notifier);
	if (rtn) {
		sl_media_log_err(NULL, LOG_NAME, "register jack event notifier failed [%d]", rtn);
		return rtn;
	}

	/*
	 * Making sure we did not miss any remove or
	 * online event during the window between
	 * first scan and notification registration
	 */
	for (jack_cntr = 0; jack_cntr < SL_MEDIA_MAX_JACK_NUM; ++jack_cntr) {
		media_jack = sl_media_data_jack_get(ldev_num, jack_cntr);
		rtn = hsnxcvr_status_get(media_jack->hdl, &media_jack->status_data);
		if (rtn) {
			sl_media_log_dbg(media_jack, LOG_NAME, "status_get failed (jack_num = %u)",
					media_jack->physical_num);
			continue;
		}

		is_removed = sl_media_data_jack_remove_verify(media_jack);
		if (is_removed)
			continue;

		sl_media_data_jack_online_verify(media_jack, ldev_num);
	}

	return 0;
}

void sl_media_data_jack_unregister_event_notifier(void)
{
	unregister_hsnxcvr_notifier(&event_notifier);
}

int sl_media_data_jack_online(void *hdl, u8 ldev_num, u8 jack_num)
{
	int                   rtn;
	u32                   flags;
	u8                    i;
	struct sl_media_jack *media_jack;
	struct sl_media_attr  media_attr;
	unsigned long         irq_flags;
	u8                    state;

	media_jack = sl_media_data_jack_get(ldev_num, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "media data jack online");

	while (sl_media_data_jack_cable_is_going_online(media_jack)) {
		spin_lock_irqsave(&media_jack->data_lock, irq_flags);
		state = media_jack->state;
		spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
		if (state == SL_MEDIA_JACK_CABLE_REMOVED) {
			sl_media_log_dbg(media_jack, LOG_NAME,
				"cable removed while waiting for online");
			return 0;
		}
		msleep(20);
	}

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->state = SL_MEDIA_JACK_CABLE_GOING_ONLINE;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	media_jack->hdl = hdl;

	rtn = hsnxcvr_jack_get(media_jack->hdl, &media_jack->jack_data);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "jack get failed [%d]", rtn);
		return rtn;
	}

	rtn = hsnxcvr_status_get(media_jack->hdl, &media_jack->status_data);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "status get failed [%d]", rtn);
		return rtn;
	}

	memset(&media_attr, 0, sizeof(struct sl_media_attr));
	switch (media_jack->jack_data.jack_type) {
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
		media_jack->i2c_data.addr   = 0;
		media_jack->i2c_data.page   = 0;
		media_jack->i2c_data.bank   = 0;
		media_jack->i2c_data.offset = 0;
		media_jack->i2c_data.len    = 256;

		rtn = hsnxcvr_i2c_read(media_jack->hdl, &media_jack->i2c_data);
		switch (rtn) {
		case -EAGAIN:
			sl_media_log_dbg(media_jack, LOG_NAME, "i2c read failed - expect online event later [%d]", rtn);
			return rtn;
		case 0:
			break;
		default:
			sl_media_log_err(media_jack, LOG_NAME, "i2c read failed [%d]", rtn);
			return rtn;
		}

		memcpy(media_jack->eeprom_page0, media_jack->i2c_data.data, SL_MEDIA_EEPROM_PAGE_SIZE);

		rtn = sl_media_data_jack_eeprom_page1_get(media_jack);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "eeprom page1 get failed [%d]", rtn);
			return rtn;
		}

		rtn = sl_media_eeprom_parse(media_jack, &media_attr);
		if (rtn) {
			sl_media_log_err(media_jack, LOG_NAME, "eeprom parse failed [%d]", rtn);
			if (sl_media_jack_is_cable_format_invalid(media_jack)) {
				memset(&media_attr, 0, sizeof(struct sl_media_attr));
				media_attr.options |= SL_MEDIA_OPT_CABLE_FORMAT_INVALID;
				for (i = 0; i < media_jack->jack_data.port_count; ++i) {
					media_jack->cable_info[i].ldev_num = ldev_num;
					media_jack->cable_info[i].lgrp_num = media_jack->jack_data.asic_port[i];
					rtn = sl_media_data_jack_media_attr_set(media_jack, &media_jack->cable_info[i],
										&media_attr);
					if (rtn) {
						sl_media_log_err(media_jack, LOG_NAME, "media_attr set failed [%d]",
								 rtn);
						return rtn;
					}
				}
			}
			return -EFAULT;
		}
		rtn = sl_media_data_cable_db_ops_cable_validate(&media_attr, media_jack);
		if (rtn) {
			sl_media_log_warn(media_jack, LOG_NAME, "cable validate failed [%d]", rtn);
			sl_media_log_warn(media_jack, LOG_NAME,
				"media attrs (vendor = %d %s, type = 0x%X %s, length_cm = %d, speeds_map = 0x%X)",
				media_attr.vendor, sl_media_vendor_str(media_attr.vendor),
				media_attr.type, sl_media_type_str(media_attr.type),
				media_attr.length_cm, media_attr.speeds_map);
			media_jack->is_cable_not_supported = true;
			media_attr.options |= SL_MEDIA_OPT_CABLE_NOT_SUPPORTED;
		}
	} else {
		media_attr.magic         = SL_MEDIA_ATTR_MAGIC;
		media_attr.ver           = SL_MEDIA_ATTR_VER;
		media_attr.vendor        = SL_MEDIA_VENDOR_HPE;
		media_attr.type          = SL_MEDIA_TYPE_BKP;
		media_attr.options       = SL_MEDIA_OPT_AUTONEG;
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
		strncpy(media_attr.serial_num, "AK20212120", sizeof(media_attr.serial_num));
		strncpy(media_attr.hpe_pn_str, "PK60821-555", sizeof(media_attr.hpe_pn_str));
	}

	flags = 0;
	if (media_attr.vendor == SL_MEDIA_VENDOR_MULTILANE)
		flags |= SL_MEDIA_TYPE_LOOPBACK;
	if (media_attr.jack_type == SL_MEDIA_JACK_TYPE_BACKPLANE)
		flags |= SL_MEDIA_TYPE_BACKPLANE;
	if (media_jack->is_cable_not_supported)
		flags |= SL_MEDIA_TYPE_NOT_SUPPORTED;

	rtn = sl_media_data_cable_db_ops_serdes_settings_get(media_jack, flags);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "serdes settings get failed [%d]", rtn);
		return rtn;
	}

	for (i = 0; i < media_jack->jack_data.port_count; ++i) {
		media_jack->cable_info[i].ldev_num = ldev_num;
		media_jack->cable_info[i].lgrp_num = media_jack->jack_data.asic_port[i];
		rtn = sl_media_data_jack_media_attr_set(media_jack, &media_jack->cable_info[i], &media_attr);
		if (rtn) {
			sl_media_log_err(media_jack, LOG_NAME, "media_attr set failed [%d]", rtn);
			return rtn;
		}
	}

	if (media_attr.type == SL_MEDIA_TYPE_AOC ||
		media_attr.type == SL_MEDIA_TYPE_AEC) {
		rtn = sl_media_jack_cable_high_power_set(ldev_num, jack_num);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "high power set failed [%d]", rtn);
			return rtn;
		}
	}

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->state = SL_MEDIA_JACK_CABLE_ONLINE;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	sl_media_log_dbg(media_jack, LOG_NAME, "online (jack = 0x%p)", media_jack);

	return 0;
}

int sl_media_data_jack_lgrp_connect(struct sl_media_lgrp *media_lgrp)
{
	struct sl_media_jack *media_jack;
	u8                    jack_num;
	u8                    i;

	sl_media_log_dbg(media_lgrp, LOG_NAME,
		"jack lgrp connect (lgrp_num = %u)", media_lgrp->num);

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_data_jack_get(media_lgrp->media_ldev->num, jack_num);
		for (i = 0; i < media_jack->jack_data.port_count; ++i) {
			if (media_jack->jack_data.asic_port[i] == media_lgrp->num) {
				sl_media_log_dbg(media_lgrp, LOG_NAME,
					"jack lgrp connect found (jack_num = %u, asic_port = %u)",
					jack_num, media_jack->jack_data.asic_port[i]);
				media_lgrp->media_jack = media_jack;
				media_lgrp->cable_info = &media_jack->cable_info[i];
				return 0;
			}
		}
	}

	sl_media_log_dbg(media_lgrp, LOG_NAME, "jack lgrp connect not found");
	return -EFAULT;
}