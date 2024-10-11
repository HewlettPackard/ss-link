// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include "string.h"

#include "linktool.h"
#include "linktool_addr.h"

void addr_init(addr_t *addr_struct)
{
    memset(addr_struct, 0, sizeof(addr_t));

    addr_struct->lane = SBUS_ADDR_IGNORE_LANE;
}

unsigned int make_addr3( int chip, int ring, int sbus)
{
    addr_t addr_struct;

    addr_init(&addr_struct);

    addr_struct.chip = chip;
    addr_struct.ring = ring;
    addr_struct.sbus = sbus & 0xff;

    return struct_to_addr(&addr_struct);
}

unsigned int make_addr4( int chip, int ring, int sbus, int lane)
{
    addr_t addr_struct;

    addr_init(&addr_struct);

    addr_struct.chip = chip;
    addr_struct.ring = ring;
    addr_struct.sbus = sbus & 0xFF;
    addr_struct.lane = lane;

    return struct_to_addr(&addr_struct);
}

unsigned int struct_to_addr(addr_t *addr_struct)
{
    unsigned int addr, lane;

    switch( addr_struct->lane ) {
    case SBUS_ADDR_IGNORE_LANE: lane = 0; break;
    case SBUS_ADDR_QUAD_LOW:    lane = 1; break;
    case SBUS_ADDR_QUAD_HIGH:   lane = 2; break;
    case SBUS_ADDR_QUAD_ALL:    lane = 3; break;
    case SBUS_ADDR_PAIR_0:      lane = 4; break;
    case SBUS_ADDR_PAIR_1:      lane = 5; break;
    case SBUS_ADDR_PAIR_2:      lane = 6; break;
    case SBUS_ADDR_PAIR_3:      lane = 7; break;
    case 0: case 1: case 2: case 3:
    case 4: case 5: case 6: case 7: lane = 8 + addr_struct->lane; break;
    default: lane = 0;
    }

    addr = (addr_struct->chip & 0x0f) << 12 |
           (addr_struct->ring & 0x0f) <<  8 |
           (addr_struct->sbus & 0xff) <<  0 |
           (             lane       ) << 16 |
           (addr_struct->pll  & 0x07) << 24;

    return addr;
}

int addr_to_struct(unsigned int addr, addr_t *addr_struct)
{
    unsigned char lane_table[16] = { SBUS_ADDR_IGNORE_LANE,
	    SBUS_ADDR_QUAD_LOW, SBUS_ADDR_QUAD_HIGH, SBUS_ADDR_QUAD_ALL,
            SBUS_ADDR_PAIR_0, SBUS_ADDR_PAIR_1, SBUS_ADDR_PAIR_2, SBUS_ADDR_PAIR_3,
                                     0, 1, 2, 3, 4, 5, 6, 7 };
    addr_init(addr_struct);

    addr_struct->chip = (addr >> 12) & 0x0f;
    addr_struct->ring = (addr >>  8) & 0x0f;
    addr_struct->sbus =  addr        & 0xff;
    addr_struct->lane = lane_table[(addr >> 16) & 0x0f];
    addr_struct->pll  = (addr >> 24) & 0x07;

    if( addr_struct->chip == 0x0f )
	    addr_struct->chip = SBUS_ADDR_BROADCAST;
    if( addr_struct->ring == 0x0f )
	    addr_struct->ring = SBUS_ADDR_BROADCAST;
    if( addr_struct->sbus == 0xff )
	    addr_struct->sbus = SBUS_ADDR_BROADCAST;

    return (addr & 0xf8f00000) == 0;
}
