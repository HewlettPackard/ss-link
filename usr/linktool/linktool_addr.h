/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_ADDR_H_
#define _LINKTOOL_ADDR_H_

// FIXME: scrub out anything looking like broadcom

#define MAX_CHIPS 2
#define MAX_RINGS 5

#define SBUS_CONTROLLER_ADDRESS  (0xfe)

#define IP_REV_7NM    0xc5
#define TSMC_07 2
#define OSPREY  0x2d

#define    SBUS_ADDR_BROADCAST       0xff
#define    SBUS_ADDR_IGNORE_LANE     0xf0
#define    SBUS_ADDR_PAIR_0          0x20
#define    SBUS_ADDR_PAIR_1          0x21
#define    SBUS_ADDR_PAIR_2          0x22
#define    SBUS_ADDR_PAIR_3          0x23
#define    SBUS_ADDR_QUAD_LOW        0x40
#define    SBUS_ADDR_QUAD_HIGH       0x41
#define    SBUS_ADDR_QUAD_ALL        0xff
#define    SBUS_ADDR_SINGLE_LANE     0xfe

#define SBUS_BROADCAST                (SBUS_ADDR_BROADCAST)

typedef struct _addr_t {
    unsigned char chip;
    unsigned char ring;
    unsigned char sbus;
    unsigned char lane;
    unsigned char pll;
    unsigned int  results;
    unsigned int  group_addr;
} addr_t;

typedef struct _info_t {
    unsigned int chips;
    unsigned int rings;

    unsigned short ip_rev[MAX_CHIPS][MAX_RINGS][256];
    unsigned char  process_id[MAX_CHIPS][MAX_RINGS];
    unsigned char  max_dev_addr[MAX_CHIPS][MAX_RINGS];
    unsigned char  dev_addr[MAX_CHIPS][MAX_RINGS];
} info_t;

void addr_init(addr_t *addr_struct);
unsigned int make_addr3( int chip, int ring, int sbus);
unsigned int make_addr4( int chip, int ring, int sbus, int lane);
unsigned int struct_to_addr(addr_t *addr_struct);
int addr_to_struct(unsigned int addr, addr_t *addr_struct);

#endif /* _LINKTOOL_ADDR_H_ */
