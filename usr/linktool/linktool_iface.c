// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>

#include "linktool_addr.h"

#include "linktool.h"
#include "linktool_iface.h"

#include <sbl_kconfig.h>
#include <hms_sbl.h>
#include <sbl_sbm_serdes_iface.h>

struct hms_sbl_inst sbl;

static unsigned int is_iface_open = 0;
static unsigned int iface_debug   = 0;

void linktool_iface_debug_set()
{
	iface_debug = 1;
}

int linktool_iface_open_fn()
{
        int rtn;

        DEBUG(iface_debug, "iface_open_fn");

        if (is_iface_open)
                return 0;

        rtn = hms_sbl_init(&sbl);
        if (rtn != 0) {
                ERROR("iface_open-fn - hms_sbl_init failed [%d]", rtn);
                return rtn;
        }

        is_iface_open = 1;

        return 0;
}

unsigned int linktool_iface_sbus_fn(unsigned int addr,
        unsigned char reg, unsigned char command, unsigned int *data)
{
        int          rtn;
        unsigned int result;

        DEBUG(iface_debug, "iface_sbus_fn");

        if (!is_iface_open) {
                ERROR("iface_sbus_fn - sbus not open");
                return -1;
        }

        command |= LINKTOOL_IFACE_CORE;
        rtn = sbl_sbus_op_aux(&sbl, addr, reg, command, *data, &result);
        if (rtn != 0) {
                ERROR("iface_sbus_fn - sbl_sbus_op_aux failed [%d]", rtn);
                return rtn;
        }

        *data = result;

        DEBUG(iface_debug, "iface_sbus_fn - data = 0x%08X", *data);

        return 0;
}

int linktool_iface_close_fn()
{
        int rtn;

        DEBUG(iface_debug, "iface_close_fn");

        if (!is_iface_open)
                return 0;

        rtn = hms_sbl_teardown(&sbl);
        if (rtn != 0) {
                ERROR("iface_close_fn - hms_sbl_teardown failed [%d]", rtn);
                return rtn;
        }

        is_iface_open = 0;

        return 0;
}
