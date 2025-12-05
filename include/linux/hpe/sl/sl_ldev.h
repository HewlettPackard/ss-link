/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_LDEV_H_
#define _LINUX_SL_LDEV_H_

#include <linux/kobject.h>
#include <linux/workqueue.h>

struct sl_dt_lgrp_info;

#define SL_LDEV_POLICY_MAGIC 0x736c6470
#define SL_LDEV_POLICY_VER   1
struct sl_ldev_policy {
	u32 magic;
	u32 ver;
	u32 size;

	u32 options;
};

typedef void (*sl_intr_handler_t)(u64 *err_flags, int flag_count, void *data);

/* ASIC types */
typedef u64 (*sl_read64_t)(void *pci_accessor, long addr);
typedef void (*sl_write64_t)(void *pci_accessor, long addr, u64 data);

/* Sbus types */
#define SL_SBUS_OP_RST  1
#define SL_SBUS_OP_RD   2
#define SL_SBUS_OP_WR   3
typedef int (*sl_sbus_op_t)(void *sbus_accessor, u8 op,
			    u8 ring, u8 dev_addr, u8 reg, u32 *rd_data, u32 wr_data);

/* PMI types */
#define SL_PMI_OP_RST  1
#define SL_PMI_OP_RD   2
#define SL_PMI_OP_WR   3
typedef int (*sl_pmi_op_t)(void *pmi_accessor, u8 lgrp_num, u8 op,
			   u32 addr, u16 *rd_data, u16 wr_data, u16 wr_data_mask);

/* PML Intr types */
typedef int (*sl_pml_intr_register_t)(void *intr_accessor, u32 lgrp_num, char *tag,
				      u64 *err_flgs, sl_intr_handler_t handler, void *data);
typedef int (*sl_pml_intr_unregister_t)(void *intr_accessor, u32 lgrp_num,
				      u64 *err_flgs, sl_intr_handler_t handler);
typedef int (*sl_pml_intr_enable_t)(void *intr_accessor, u32 lgrp_num,
				    u64 *err_flgs, sl_intr_handler_t handler);
typedef int (*sl_pml_intr_disable_t)(void *intr_accessor, u32 lgrp_num,
				     u64 *err_flgs, sl_intr_handler_t handler);

/* DT types */
typedef int (*sl_dt_info_get_t)(void *dt_accessor, u8 ldev_num, u8 lgrp_num,
				struct sl_dt_lgrp_info *info);

/* UC types */
typedef int (*sl_uc_rd_t)(void *uc_accessor, u32 offset, u32 page, u8 *data, u32 len);
typedef int (*sl_uc_wr8_t)(void *uc_accessor, u8 page, u8 addr, u8 data);

typedef void (*sl_uc_led_set_t)(void *uc_accessor, u8 led_pattern);

/* MB types */
typedef int (*sl_mb_info_get_t)(void *mb_accessor, u8 *platform, u16 *revision, u16 *proto);

/* DMAC types */
typedef int  (*sl_dmac_alloc_t)(void *dmac_accessor, u32 offset, size_t size);
typedef int  (*sl_dmac_xfer_t)(void *dmac_accessor, void *data);
typedef void (*sl_dmac_free_t)(void *dmac_accessor);

struct sl_accessors {
	void *pci;
	void *sbus;
	void *pmi;
	void *intr;
	void *dt;
	void *mb;
	void *dmac;
};

struct sl_uc_accessor {
	void *uc;
};

struct sl_uc_ops {
	sl_uc_rd_t      uc_read;
	sl_uc_wr8_t     uc_write8;
	sl_uc_led_set_t uc_led_set;
};

struct sl_ops {
	/* ASIC */
	sl_read64_t  read64;
	sl_write64_t write64;

	/* Sbus */
	sl_sbus_op_t sbus_op;

	/* PMI */
	sl_pmi_op_t pmi_op;

	/* PML interrupts */
	sl_pml_intr_register_t   pml_intr_register;
	sl_pml_intr_unregister_t pml_intr_unregister;
	sl_pml_intr_enable_t     pml_intr_enable;
	sl_pml_intr_disable_t    pml_intr_disable;

	/* DT */
	sl_dt_info_get_t dt_info_get;

	/* MB */
	sl_mb_info_get_t mb_info_get;

	sl_dmac_alloc_t dmac_alloc;
	sl_dmac_xfer_t  dmac_xfer;
	sl_dmac_free_t  dmac_free;
};

#define SL_LDEV_ATTR_MAGIC 0x736c6461
#define SL_LDEV_ATTR_VER   1
struct sl_ldev_attr {
	u32 magic;
	u32 ver;
	u32 size;

	struct sl_ops       *ops;
	struct sl_accessors *accessors;

	u32 options;
};

struct sl_ldev *sl_ldev_new(u8 ldev_num, struct workqueue_struct *workq,
			    struct sl_ldev_attr *ldev_attr);
int             sl_ldev_sysfs_parent_set(struct sl_ldev *ldev, struct kobject *parent);
int             sl_ldev_serdes_init(struct sl_ldev *ldev);
int             sl_ldev_del(struct sl_ldev *ldev);
int             sl_ldev_uc_ops_set(struct sl_ldev *ldev, struct sl_uc_ops *uc_ops,
				   struct sl_uc_accessor *uc_accessor);

#endif /* _LINUX_SL_LDEV_H_ */

