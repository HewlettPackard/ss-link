/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_LGRP_H_
#define _LINUX_SL_LGRP_H_

#include <linux/bitops.h>
#include <linux/kobject.h>

#include <linux/hpe/sl/sl_link.h>
#include <linux/hpe/sl/sl_llr.h>
#include <linux/hpe/sl/sl_media.h>

struct sl_ldev;
struct sl_lgrp;
struct sl_lgrp_config;

#define SL_LGRP_NOTIF_INVALID           0
#define SL_LGRP_NOTIF_ALL               ~0       /* all notifications               */
#define SL_LGRP_NOTIF_LINK_UP           BIT(1)   /* link up                         */
#define SL_LGRP_NOTIF_LINK_UP_FAIL      BIT(2)   /* link up fail                    */
#define SL_LGRP_NOTIF_LINK_DOWN         BIT(3)   /* link down                       */
#define SL_LGRP_NOTIF_LINK_ASYNC_DOWN   BIT(4)   /* link down asynchronously        */
#define SL_LGRP_NOTIF_LINK_ERROR        BIT(5)   /* link error                      */
#define SL_LGRP_NOTIF_LINK_UCW_WARN     BIT(6)   /* UCW count crossed warning limit */
#define SL_LGRP_NOTIF_LINK_CCW_WARN     BIT(7)   /* CCW count crossed warning limit */
#define SL_LGRP_NOTIF_LLR_SETUP         BIT(8)   /* LLR is setup + data             */
#define SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT BIT(9)   /* LLR setup timeout               */
#define SL_LGRP_NOTIF_LLR_RUNNING       BIT(10)  /* LLR running                     */
#define SL_LGRP_NOTIF_LLR_START_TIMEOUT BIT(11)  /* LLR start timeout               */
#define SL_LGRP_NOTIF_LLR_CANCELED      BIT(12)  /* LLR canceled                    */
#define SL_LGRP_NOTIF_LLR_ERROR         BIT(13)  /* LLR error                       */
#define SL_LGRP_NOTIF_MEDIA_PRESENT     BIT(14)  /* media/cable present             */
#define SL_LGRP_NOTIF_MEDIA_NOT_PRESENT BIT(15)  /* media/cable not present         */
#define SL_LGRP_NOTIF_MEDIA_ERROR       BIT(16)  /* media/cable error               */
#define SL_LGRP_NOTIF_AN_DATA           BIT(17)  /* autoneg data                    */
#define SL_LGRP_NOTIF_AN_TIMEOUT        BIT(18)  /* autoneg timeout                 */
#define SL_LGRP_NOTIF_AN_ERROR          BIT(19)  /* autoneg error                   */
#define SL_LGRP_NOTIF_LANE_DEGRADE      BIT(20)  /* auto lane degrade               */
#define SL_LGRP_NOTIF_MEDIA_HIGH_TEMP   BIT(21)  /* media/cable high temp detected  */

#define SL_LGRP_NOTIF_NO_LINK 0xFF

#define SL_LGRP_NOTIF_LINK     (SL_LGRP_NOTIF_LINK_UP         | \
				SL_LGRP_NOTIF_LINK_UP_FAIL    | \
				SL_LGRP_NOTIF_LINK_DOWN       | \
				SL_LGRP_NOTIF_LINK_ASYNC_DOWN | \
				SL_LGRP_NOTIF_LINK_ERROR      | \
				SL_LGRP_NOTIF_LINK_UCW_WARN   | \
				SL_LGRP_NOTIF_LINK_CCW_WARN)

#define SL_LGRP_NOTIF_LLR      (SL_LGRP_NOTIF_LLR_SETUP         | \
				SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT | \
				SL_LGRP_NOTIF_LLR_RUNNING       | \
				SL_LGRP_NOTIF_LLR_START_TIMEOUT | \
				SL_LGRP_NOTIF_LLR_CANCELED      | \
				SL_LGRP_NOTIF_LLR_ERROR)

#define SL_LGRP_NOTIF_MEDIA    (SL_LGRP_NOTIF_MEDIA_PRESENT     | \
				SL_LGRP_NOTIF_MEDIA_NOT_PRESENT | \
				SL_LGRP_NOTIF_MEDIA_HIGH_TEMP   | \
				SL_LGRP_NOTIF_MEDIA_ERROR)

#define SL_LGRP_NOTIF_AN       (SL_LGRP_NOTIF_AN_DATA    | \
				SL_LGRP_NOTIF_AN_TIMEOUT | \
				SL_LGRP_NOTIF_AN_ERROR)

#define SL_LGRP_CONFIG_TECH_CK_400G      BIT(18)
#define SL_LGRP_CONFIG_TECH_CK_200G      BIT(17)
#define SL_LGRP_CONFIG_TECH_CK_100G      BIT(16)
#define SL_LGRP_CONFIG_TECH_BS_200G      BIT(15)
#define SL_LGRP_CONFIG_TECH_CD_100G      BIT(14)
#define SL_LGRP_CONFIG_TECH_CD_50G       BIT(13)
#define SL_LGRP_CONFIG_TECH_BJ_100G      BIT(8)

#define SL_LGRP_CONFIG_TECH_MASK (SL_LGRP_CONFIG_TECH_CK_400G | \
				  SL_LGRP_CONFIG_TECH_CK_200G | \
				  SL_LGRP_CONFIG_TECH_CK_100G | \
				  SL_LGRP_CONFIG_TECH_BS_200G | \
				  SL_LGRP_CONFIG_TECH_CD_100G | \
				  SL_LGRP_CONFIG_TECH_CD_50G  | \
				  SL_LGRP_CONFIG_TECH_BJ_100G)

#define SL_LGRP_CONFIG_FEC_RS_LL         BIT(1) // FIXME: need to fix how we handle this
#define SL_LGRP_CONFIG_FEC_RS            BIT(0)

#define SL_LGRP_CONFIG_FEC_MASK (SL_LGRP_CONFIG_FEC_RS_LL | \
				 SL_LGRP_CONFIG_FEC_RS)

#define SL_LGRP_FEC_MODE_OFF     BIT(0)
#define SL_LGRP_FEC_MODE_MONITOR BIT(1)
#define SL_LGRP_FEC_MODE_CORRECT BIT(2)

#define SL_LGRP_LLR_MODE_OFF     BIT(0)
#define SL_LGRP_LLR_MODE_MONITOR BIT(1)
#define SL_LGRP_LLR_MODE_ON      BIT(2)

#define SL_LGRP_CONFIG_OPT_FABRIC                 BIT(0) /* fabric link     */
#define SL_LGRP_CONFIG_OPT_R1                     BIT(1) /* connected to R1 */
#define SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE BIT(2) /* serdes loopback */
#define SL_LGRP_CONFIG_OPT_8BYTE_PREAMBLE         BIT(3) /* 4 byte preamble */

#define SL_LGRP_CONFIG_MAGIC 0x6c676366
#define SL_LGRP_CONFIG_VER   1
struct sl_lgrp_config {
	u32 magic;
	u32 ver;
	u32 size;

	u32 mfs;
	u32 furcation;
	u32 fec_mode;

	u32 tech_map;
	u32 fec_map;

	u32 options;
};

#define SL_LGRP_POLICY_MAGIC 0x6c67706f
#define SL_LGRP_POLICY_VER   1
struct sl_lgrp_policy {
	u32 magic;
	u32 ver;
	u32 size;

	u32 options;
};

#define SL_HW_ATTR_MAGIC 0x73736861
#define SL_HW_ATTR_VER   1
struct sl_hw_attr {
        u32 magic;
        u32 ver;

        u8 cxi_num;
        u8 nic_num;
};

int sl_lgrp_hw_attr_set(struct sl_lgrp *lgrp, struct sl_hw_attr *hw_attr);

struct sl_lgrp_notif_info_link_up {
	u32 mode;
	u32 fec_type;
	u32 fec_mode;
};

union sl_lgrp_notif_info {
	struct sl_lgrp_notif_info_link_up         link_up;
	struct sl_link_caps                       lp_link_caps;
	struct sl_llr_data                        llr_data;
	struct sl_media_attr                      media_attr;
	struct sl_link_degrade_info               degrade_info;
	int                                       error;
	u64                                       cause_map;
};

struct sl_lgrp_notif_msg {
	u8                       ldev_num;
	u8                       lgrp_num;
	u8                       link_num;
	u32                      type;
	union sl_lgrp_notif_info info;
	u64                      info_map;
};

#define SL_MAX_LANES 4
struct sl_dt_lgrp_info {
	u8 sbus_ring;
	u8 dev_id;
	u8 dev_addr;
	struct {
		u8 tx_source;
		u8 tx_invert;
		u8 rx_source;
		u8 rx_invert;
	} lane_info[SL_MAX_LANES];
};

struct sl_lgrp *sl_lgrp_new(struct sl_ldev *ldev, u8 lgrp_num, struct kobject *sysfs_parent);
int             sl_lgrp_del(struct sl_lgrp *lgrp);

int sl_lgrp_config_set(struct sl_lgrp *lgrp, struct sl_lgrp_config *lgrp_config);
int sl_lgrp_policy_set(struct sl_lgrp *lgrp, struct sl_lgrp_policy *lgrp_policy);

typedef void (*sl_lgrp_notif_t)(void *tag, struct sl_lgrp_notif_msg *msg);

int sl_lgrp_notif_callback_reg(struct sl_lgrp *lgrp, sl_lgrp_notif_t callback,
			       u32 types, void *tag);
int sl_lgrp_notif_callback_unreg(struct sl_lgrp *lgrp, sl_lgrp_notif_t callback,
				 u32 types);

int sl_lgrp_connect_id_set(struct sl_lgrp *lgrp, const char *connect_id);

const char *sl_lgrp_config_tech_str(u32 config);
const char *sl_lgrp_config_fec_str(u32 config);
const char *sl_lgrp_furcation_str(u32 furcation);
const char *sl_lgrp_notif_str(u32 notif);
const char *sl_lgrp_config_opt_str(u32 option);
const char *sl_lgrp_fec_mode_str(u32 mode);
const char *sl_lgrp_llr_mode_str(u32 mode);

#endif /* _LINUX_SL_LGRP_H_ */
