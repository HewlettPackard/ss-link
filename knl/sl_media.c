// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/sl_media.h>

#include "sl_log.h"
#include "sl_lgrp.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_MEDIA_LOG_NAME

const char *sl_media_furcation_str(u32 furcation)
{
	switch (furcation) {
	case SL_MEDIA_FURCATION_X1:
		return "unfurcated";
	case SL_MEDIA_FURCATION_X2:
		return "bifurcated";
	case SL_MEDIA_FURCATION_X4:
		return "quadfurcated";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_media_furcation_str);

const char *sl_media_state_str(u8 state)
{
	switch (state) {
	case SL_MEDIA_JACK_CABLE_REMOVED:
		return "removed";
	case SL_MEDIA_JACK_CABLE_INSERTED:
		return "inserted";
	case SL_MEDIA_JACK_CABLE_GOING_ONLINE:
		return "going online";
	case SL_MEDIA_JACK_CABLE_ONLINE:
		return "online";
	case SL_MEDIA_JACK_CABLE_ERROR:
		return "error";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_media_state_str);

const char *sl_media_type_str(u32 type)
{
	switch (type) {
	case SL_MEDIA_TYPE_HEADSHELL:
		return "headshell";
	case SL_MEDIA_TYPE_BACKPLANE:
		return "backplane";
	case SL_MEDIA_TYPE_ELECTRICAL:
		return "electrical";
	case SL_MEDIA_TYPE_OPTICAL:
		return "optical";
	case SL_MEDIA_TYPE_PASSIVE:
		return "passive";
	case SL_MEDIA_TYPE_ACTIVE:
		return "active";
	case SL_MEDIA_TYPE_ANALOG:
		return "analog";
	case SL_MEDIA_TYPE_DIGITAL:
		return "digital";
	default:
		if (type == SL_MEDIA_TYPE_AOC)
			return "AOC";
		if (type == SL_MEDIA_TYPE_PEC)
			return "PEC";
		if (type == SL_MEDIA_TYPE_AEC)
			return "AEC";
		if (type == SL_MEDIA_TYPE_BKP)
			return "backplane";
		else
			return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_media_type_str);

const char *sl_media_vendor_str(u32 vendor)
{
	switch (vendor) {
	case SL_MEDIA_VENDOR_INVALID:
		return "invalid";
	case SL_MEDIA_VENDOR_UNKNOWN:
		return "unknown";
	case SL_MEDIA_VENDOR_TE:
		return "te";
	case SL_MEDIA_VENDOR_HISENSE:
		return "hisense";
	case SL_MEDIA_VENDOR_LEONI:
		return "leoni";
	case SL_MEDIA_VENDOR_FINISAR:
		return "finisar";
	case SL_MEDIA_VENDOR_MOLEX:
		return "molex";
	case SL_MEDIA_VENDOR_BIZLINK:
		return "bizlink";
	case SL_MEDIA_VENDOR_HPE:
		return "hpe";
	case SL_MEDIA_VENDOR_CLOUD_LIGHT:
		return "cloud light";
	case SL_MEDIA_VENDOR_MULTILANE:
		return "multilane";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_media_vendor_str);

const char *sl_media_speed_str(u32 speed)
{
	switch (speed) {
	case SL_MEDIA_SPEEDS_SUPPORT_CK_800G:
		return "ck800G";
	case SL_MEDIA_SPEEDS_SUPPORT_CK_400G:
		return "ck400G";
	case SL_MEDIA_SPEEDS_SUPPORT_CK_200G:
		return "ck200G";
	case SL_MEDIA_SPEEDS_SUPPORT_CK_100G:
		return "ck100G";
	case SL_MEDIA_SPEEDS_SUPPORT_BS_200G:
		return "bs200G";
	case SL_MEDIA_SPEEDS_SUPPORT_BJ_100G:
		return "bj100G";
	case SL_MEDIA_SPEEDS_SUPPORT_CD_100G:
		return "cd100G";
	case SL_MEDIA_SPEEDS_SUPPORT_CD_50G:
		return "cd50G";
	default:
		return "unrecognized";
	}
}
EXPORT_SYMBOL(sl_media_speed_str);

const char *sl_media_ber_str(u8 media_interface)
{
	switch (media_interface) {
	case 1:
		return "< 1E-12";
	case 2:
		return "< 5E-5";
	case 3:
		return "< 2.6E-4";
	case 4:
		return "< 1E-6";
	default:
		return "undefined";
	}
}
EXPORT_SYMBOL(sl_media_ber_str);

const char *sl_media_host_interface_str(u32 speed, u32 type)
{
	if ((type == SL_MEDIA_TYPE_PEC) || (type == SL_MEDIA_TYPE_BKP)) {
		switch (speed) {
		case SL_MEDIA_SPEEDS_SUPPORT_CD_50G:
			return "50GBASE-CR";
		case SL_MEDIA_SPEEDS_SUPPORT_CD_100G:
			return "100GBASE-CR2";
		case SL_MEDIA_SPEEDS_SUPPORT_CK_100G:
			return "100GBASE-CR1";
		case SL_MEDIA_SPEEDS_SUPPORT_BS_200G:
			return "200GBASE-CR4";
		case SL_MEDIA_SPEEDS_SUPPORT_CK_200G:
			return "200GBASE-CR2";
		case SL_MEDIA_SPEEDS_SUPPORT_CK_400G:
			return "400GBASE-CR4";
		case SL_MEDIA_SPEEDS_SUPPORT_CK_800G:
			return "800GBASE-CR8";
		case SL_MEDIA_SPEEDS_SUPPORT_BJ_100G:
			return "BJ 100G";
		default:
			return "unrecognized";
		}
	}

	switch (speed) {
	case SL_MEDIA_SPEEDS_SUPPORT_CD_50G:
		return "50GAUI-1 C2M";
	case SL_MEDIA_SPEEDS_SUPPORT_CD_100G:
		return "100GAUI-2 C2M";
	case SL_MEDIA_SPEEDS_SUPPORT_CK_100G:
		return "100GAUI-1-L C2M";
	case SL_MEDIA_SPEEDS_SUPPORT_BS_200G:
		return "200GAUI-4 C2M";
	case SL_MEDIA_SPEEDS_SUPPORT_CK_200G:
		return "200GAUI-2-S C2M";
	case SL_MEDIA_SPEEDS_SUPPORT_CK_400G:
		return "400GAUI-4-S C2M";
	case SL_MEDIA_SPEEDS_SUPPORT_CK_800G:
		return "800GAUI-8-S C2M";
	case SL_MEDIA_SPEEDS_SUPPORT_BJ_100G:
		return "BJ 100G";
	default:
		return "unrecognized";
	}

}
EXPORT_SYMBOL(sl_media_host_interface_str);

const char *sl_media_opt_str(u32 option)
{
	switch (option) {
	case SL_MEDIA_OPT_AUTONEG:
		return "autoneg";
	case SL_MEDIA_OPT_FAKE:
		return "fake";
	case SL_MEDIA_OPT_RESETTABLE:
		return "resettable";
	case SL_MEDIA_OPT_LINKTRAIN:
		return "linktrain";
	default:
		return "unrecognized";
	}

}
EXPORT_SYMBOL(sl_media_opt_str);

const char *sl_media_jack_type_str(u32 jack_type, u32 density)
{
	switch (jack_type) {
	case SL_MEDIA_JACK_TYPE_BACKPLANE:
		return "backplane";
	case SL_MEDIA_JACK_TYPE_SFP:
		return "sfp";
	case SL_MEDIA_JACK_TYPE_QSFP:
		if (density == SL_MEDIA_QSFP_DENSITY_DOUBLE)
			return "qsfp-dd";
		return "qsfp";
	case SL_MEDIA_JACK_TYPE_OSFP:
		return "osfp";
	default:
		return "unrecognized";
	}

}
EXPORT_SYMBOL(sl_media_jack_type_str);
