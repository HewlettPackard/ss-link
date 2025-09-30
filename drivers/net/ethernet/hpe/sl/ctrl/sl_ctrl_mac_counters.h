/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_MAC_COUNTERS_H_
#define _SL_CTRL_MAC_COUNTERS_H_

#include <linux/types.h>
#include <linux/atomic.h>

struct sl_ctrl_mac;

enum sl_ctrl_mac_counters {
	MAC_TX_START_CMD,         /* command to start mac tx   */
	MAC_TX_STARTED,           /* mac tx is started         */
	MAC_TX_START_FAIL,        /* mac tx start is failed    */
	MAC_TX_STOP_CMD,          /* comamnd to stop mac tx    */
	MAC_TX_STOPPED,           /* mac tx is stopped         */
	MAC_TX_STOP_FAIL,         /* mac tx stop is failed     */
	MAC_RX_START_CMD,         /* command to start mac rx   */
	MAC_RX_STARTED,           /* mac rx is started         */
	MAC_RX_START_FAIL,        /* mac rx start is failed    */
	MAC_RX_STOP_CMD,          /* command to stop mac rx    */
	MAC_RX_STOPPED,           /* mac rx is stopped         */
	MAC_RX_STOP_FAIL,         /* mac rx stop is failed     */
	MAC_RESET_CMD,            /* command to reset mac      */
	MAC_RESET,                /* mac reset is succeeded    */
	MAC_RESET_FAIL,           /* mac reset is failed       */
	SL_CTRL_MAC_COUNTERS_COUNT
};

struct sl_ctrl_mac_counter {
	atomic_t  count;
	char     *name;
};

#define SL_CTRL_MAC_COUNTER_INC(_mac, _counter) \
	atomic_inc(&(_mac)->counters[_counter].count)

int  sl_ctrl_mac_counters_init(struct sl_ctrl_mac *ctrl_mac);
void sl_ctrl_mac_counters_del(struct sl_ctrl_mac *ctrl_mac);
int  sl_ctrl_mac_counter_get(struct sl_ctrl_mac *ctrl_mac, u32 counter);

#endif /* _SL_CTRL_MAC_COUNTERS_H_ */
