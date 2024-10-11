// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include "linktool.h"
#include "linktool_sbus.h"

#define PMI_CHECK_REG                 1
#define PMI_ADDR_REG                  2
#define PMI_WR_DATA_REG               3
#define PMI_RD_DATA_REG               4
#define PMI_DEVID_REG                33

static unsigned int pmi_debug = 0;

void linktool_pmi_debug_set()
{
	pmi_debug = 1;
}

#if 0 // FIXME: remove this
        {
            uint reg33;
            EFUN(blackhawk_sbus_rd(sa__, 33, &reg33));
            if( reg33 & (1U << 31) )  /* test for override */
                devid_strap = (reg33 >> 24) & 0x1F;
            else
                devid_strap = (reg33 >> 16) & 0x1F;
        }
        pmi_reg_addr = (reg_addr & 0xffff) | lane_idx << 16 | pll_idx << 24 | devid_strap << 27;

        EFUN(blackhawk_sbus_wr(sa__, PMI_ADDR, pmi_reg_addr)); /* Write reg_addr, lane, pll/micro, devid address register */
#endif
static int pmi_addr_set(unsigned int dev_addr, unsigned int lane, unsigned int pll, unsigned int reg)
{
	int          rtn;
	unsigned int data32;
	unsigned int devid;
	unsigned int pmi_data;

	rtn = linktool_sbus_rd(dev_addr, PMI_DEVID_REG, &data32);
	if (rtn != 0) {
		ERROR("pmi_addr_set - sbus_rd failed [%d]", rtn);
		return rtn;
	}

	if ((data32 & (1 << 31)) != 0)
		devid = (data32 >> 24) & 0x1F;
	else
		devid = (data32 >> 16) & 0x1F;

	pmi_data = ((reg & 0xFFFF) | ((lane & 0xFF) << 16) | (pll << 24) | (devid << 27));

	// DEBUG(pmi_debug, "pmi_addr_set (pmi_data = 0x%08X)", pmi_data);

	rtn = linktool_sbus_wr(dev_addr, PMI_ADDR_REG, pmi_data);
	if (rtn != 0) {
		ERROR("pmi_addr_set - sbus_wr failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

static int pmi_data_rd(unsigned int dev_addr, unsigned int *data)
{
	int rtn;

	rtn = linktool_sbus_rd(dev_addr, PMI_RD_DATA_REG, data);

	*data &= 0xFFFF;

	return rtn;
}

static int pmi_data_wr(unsigned int dev_addr, unsigned int data, unsigned int mask)
{
	data = ((data & 0xFFFF) << 16) | ((~mask) & 0xFFFF);

	return linktool_sbus_wr(dev_addr, PMI_WR_DATA_REG, data);
}

static int pmi_check(unsigned int dev_addr)
{
	int          rtn;
	unsigned int data32;

	rtn = linktool_sbus_rd(dev_addr, PMI_CHECK_REG, &data32);
	if (rtn != 0) {
		ERROR("pmi_check - sbus_rd failed [%d]", rtn);
		return rtn;
	}

	if ((data32 & 0x8000) != 0) {
		ERROR("pmi_check - error (data = 0x%08X)", data32);
		rtn = linktool_sbus_wr(dev_addr, PMI_CHECK_REG, data32 | 0x8000);
		if (rtn != 0) {
			ERROR("pmi_check - sbus_wr failed [%d]", rtn);
			return rtn;
		}
		return -1;
	}

	return 0;
}

#if 0
            {
                uint data_32;
                EFUN(blackhawk_sbus_rd(sa__, PMI_RD_DATA, &data_32));   /* Read data from just written address. */
                SA_LOG_PRINTA(sa__, AVAGO_DEBUG0, __func__, __LINE__,  "SBus %x.%x, PMI Read(0x%04x) = 0x%08x (WA)\n", sbus_idx, lane_idx, reg_addr, data_32);

                if( (data_32 & 0x00030000) != 0 )
                {
                    uint error;
                    err_code = ERR_CODE_INVALID_RAM_ADDR;
                    EFUN(blackhawk_sbus_rd(sa__, 1, &error));   /* Check for PMI error */
                    if( error & 0x8000 )
                    {
                        sa_log_printf(sa__, AVAGO_ERR, __func__, __LINE__,  "SBus %x.%x, Read Status = 0x%x PMI Reg = 0x%x\n",
                            sbus_idx, lane_idx, error, reg_addr);
                        EFUN(blackhawk_sbus_wr(sa__, 1, error|0x8000)); /* Clear PMI error */
                    }
                }
                *data = (int16_t) data_32;
            }
#endif
int linktool_pmi_rd(unsigned int dev_addr, unsigned int lane, unsigned int pll,
	unsigned int reg, unsigned int *data)
{
	int rtn;

	DEBUG(pmi_debug, "pmi_rd (dev_addr = 0x%02X, lane = 0x%02X, pll = %d, reg = 0x%04X)",
		dev_addr, lane, pll, reg);

	*data = 0;
	rtn = pmi_addr_set(dev_addr, lane, pll, reg);
	if (rtn != 0) {
		ERROR("pmi_rd - pmi_addr_set failed [%d]", rtn);
		return rtn;
	}
	rtn = pmi_data_rd(dev_addr, data);
	if (rtn != 0) {
		ERROR("pmi_rd - pmi_data_rd failed [%d]", rtn);
		return rtn;
	}
	rtn = pmi_check(dev_addr);
	if (rtn != 0) {
		ERROR("pmi_rd - pmi_check failed [%d]", rtn);
		return rtn;
	}

	DEBUG(pmi_debug, "pmi_rd (reg = 0x%04X, data = 0x%04X)", reg, *data);

	return 0;
}

#if 0
            EFUN(blackhawk_sbus_wr(sa__, 3, *data<<16 | ((~mask) & 0xffff)));    /* Method A masked write */
            {
                uint error;
                EFUN(blackhawk_sbus_rd(sa__, 1, &error));   /* Check for PMI error */
                if( error & 0x8000 )
                {
                    sa_log_printf(sa__, AVAGO_ERR, __func__, __LINE__,  "SBus %x.%x, Reg 0x%x, PMI Masked Write Status = 0x%x\n",
                        sbus_idx, lane_idx, reg_addr, error);
                    EFUN(blackhawk_sbus_wr(sa__, 1, error|0x8000)); /* Clear PMI error */
                }
            }
#endif
int linktool_pmi_wr(unsigned int dev_addr, unsigned int lane, unsigned int pll,
	unsigned int reg, unsigned int data, unsigned int mask)
{
	int rtn;

	DEBUG(pmi_debug,
		"pmi_wr (dev_addr = 0x%02X, lane = 0x%02X, pll = %d, reg = 0x%04X, data = 0x%04X, mask = 0x%04X)",
		dev_addr, lane, pll, reg, data, mask);

	rtn = pmi_addr_set(dev_addr, lane, pll, reg);
	if (rtn != 0) {
		ERROR("pmi_wr - pmi_addr_set failed [%d]", rtn);
		return rtn;
	}

	rtn = pmi_data_wr(dev_addr, data, mask);
	if (rtn != 0) {
		ERROR("pmi_wr - pmi_data_wr failed [%d]", rtn);
		return rtn;
	}

	rtn = pmi_check(dev_addr);
	if (rtn != 0) {
		ERROR("pmi_wr - pmi_check failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
