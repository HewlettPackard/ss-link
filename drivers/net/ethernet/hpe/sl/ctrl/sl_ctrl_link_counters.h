/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LINK_COUNTERS_H_
#define _SL_CTRL_LINK_COUNTERS_H_

struct sl_ctrl_link;

enum sl_ctrl_link_counters {
	LINK_UP_CMD,                    /* command to bring the link up                       */
	LINK_UP_RETRY,                  /* link up retry                                      */
	LINK_UP,                        /* link is up                                         */
	LINK_UP_FAIL,                   /* link up failed                                     */
	LINK_DOWN_CMD,                  /* command to bring the link down                     */
	LINK_DOWN,                      /* link is down                                       */
	LINK_UP_CANCEL_CMD,             /* link up cancel commanded                           */
	LINK_UP_CANCELED,               /* link up cancel completed                           */
	LINK_RESET_CMD,                 /* command to reset the link                          */
	LINK_FAULT_ASYNC,               /* link fault async                                   */
	LINK_RECOVERING,                /* FIXME: need to implement this                      */
	LINK_CCW_WARN_CROSSED,          /* ccw warn limit crossed                             */
	LINK_UCW_WARN_CROSSED,          /* ucw warn limit crossed                             */
	LINK_DOWN_CCW_LIMIT_CROSSED,    /* link down ccw limit crossed                        */
	LINK_DOWN_UCW_LIMIT_CROSSED,    /* link down ucw limit crossed                        */
	LINK_DOWN_CCW_CAUSE,            /* ccw max chance crossed causing the link to go down */
	LINK_DOWN_UCW_CAUSE,            /* ucw max chance crossed causing the link to go down */
	LINK_UP_FAIL_CCW_LIMIT_CROSSED, /* link up failed, ucw limit crossed                  */
	LINK_UP_FAIL_UCW_LIMIT_CROSSED, /* link up failed, ccw limit crossed                  */
	LINK_HW_AN_ATTEMPT,             /* link an attempt                                    */
	SL_CTRL_LINK_COUNTERS_COUNT
};

enum sl_ctrl_link_cause_counters {
	LINK_CAUSE_UCW,               /* link up or fec mon UCW limit crossed */
	LINK_CAUSE_LF,                /* link local fault                     */
	LINK_CAUSE_RF,                /* link remote fault                    */
	LINK_CAUSE_DOWN,              /* link down fault                      */
	LINK_CAUSE_UP_TRIES,          /* link up tries exhaused               */
	LINK_CAUSE_AUTONEG_NOMATCH,   /* lp_caps autoneg no match             */
	LINK_CAUSE_AUTONEG,           /* autoneg failure                      */
	LINK_CAUSE_CONFIG,            /* link up bad config                   */
	LINK_CAUSE_INTR_ENABLE,       /* link up interrupt enable failure     */
	LINK_CAUSE_TIMEOUT,           /* link up timeout                      */
	LINK_CAUSE_CANCELED,          /* link up cancelled                    */
	LINK_CAUSE_UNSUPPORTED_CABLE, /* unsuppported cable                   */
	LINK_CAUSE_COMMAND,           /* client command                       */
	LINK_CAUSE_DOWNSHIFT,         /* link up cable downshift failed       */
	LINK_CAUSE_LLR_REPLAY_MAX,    /* LLR replay at max fault              */
	LINK_CAUSE_UPSHIFT,           /* link up cable upshift failed         */
	LINK_CAUSE_AUTONEG_CONFIG,    /* link up config after an failed       */
	LINK_CAUSE_PCS_FAULT,         /* link up PCS is not ok                */
	LINK_CAUSE_SERDES_PLL,        /* link up serdes problems              */
	LINK_CAUSE_SERDES_CONFIG,     /* link up serdes config problems       */
	LINK_CAUSE_SERDES_SIGNAL,     /* link up serdes signal problems       */
	LINK_CAUSE_SERDES_QUALITY,    /* link up serdes quality problems      */
	LINK_CAUSE_NO_MEDIA,          /* no media present                     */
	LINK_CAUSE_CCW,               /* link up or fec mon CCW limit crossed */
	LINK_CAUSE_HIGH_TEMP,         /* active cable too hot                 */
	LINK_CAUSE_INTR_REGISTER,     /* link up interrupt register failure   */
	LINK_CAUSE_MEDIA_ERROR,       /* media has errors                     */
	LINK_CAUSE_UP_CANCELED,       /* link up canceled, link down          */
	LINK_CAUSE_UNSUPPORTED_SPEED, /* unsupported speed                    */
	LINK_CAUSE_SS200_CABLE,       /* SS200 cable                          */
	LINK_CAUSE_TX_LOL,            /* TX Loss of Lock                      */
	LINK_CAUSE_RX_LOL,            /* RX Loss of Lock                      */
	LINK_CAUSE_TX_LOS,            /* TX Loss of Signal                    */
	LINK_CAUSE_RX_LOS,            /* RX Loss of Signal                    */
	SL_CTRL_LINK_CAUSE_COUNTERS_COUNT
};

enum sl_ctrl_link_an_cause_counters {
	LINK_AN_CAUSE_LP_CAPS_SERDES_LINK_UP_FAIL,
	LINK_AN_CAUSE_LP_CAPS_NOT_COMPLETE,
	LINK_AN_CAUSE_NOT_COMPLETE,
	LINK_AN_CAUSE_TEST_CAPS_NOMATCH,
	LINK_AN_CAUSE_SERDES_LINK_UP_FAIL,
	LINK_AN_CAUSE_BP_STORE_STATE_BAD,
	LINK_AN_CAUSE_BP_STORE_LP_ABILITY_NOT_SET,
	LINK_AN_CAUSE_BP_STORE_STATE_ERROR,
	LINK_AN_CAUSE_BP_STORE_BP_NOT_SET,
	LINK_AN_CAUSE_BP_SEND_INTR_ENABLE_FAIL,
	LINK_AN_CAUSE_NP_STORE_STATE_BAD,
	LINK_AN_CAUSE_NP_STORE_BP_SET,
	LINK_AN_CAUSE_NP_CHECK_STATE_BAD,
	LINK_AN_CAUSE_INTR_STATE_INVALID,
	LINK_AN_CAUSE_INTR_AN_RETRY_NP_SEND_FAIL,
	LINK_AN_CAUSE_INTR_OUT_OF_PAGES,
	LINK_AN_CAUSE_INTR_NP_SEND_FAIL,
	LINK_AN_CAUSE_PAGES_DECODE_FAIL,
	LINK_AN_CAUSE_PAGES_DECODE_NO_BP,
	LINK_AN_CAUSE_PAGES_DECODE_OUI_INVALID,
	SL_CTRL_LINK_AN_CAUSE_COUNTERS_COUNT
};

struct sl_ctrl_link_counter {
	atomic_t  count;
	char     *name;
};

#define SL_CTRL_LINK_COUNTER_INC(_link, _counter) \
	atomic_inc(&(_link)->counters[_counter].count)

#define SL_CTRL_LINK_CAUSE_COUNTER_INC(_link, _counter) \
	atomic_inc(&(_link)->cause_counters[_counter].count)

#define SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(_link, _counter) \
	atomic_inc(&(_link)->an_cause_counters[_counter].count)

int  sl_ctrl_link_counters_init(struct sl_ctrl_link *ctrl_link);
void sl_ctrl_link_counters_del(struct sl_ctrl_link *ctrl_link);
int  sl_ctrl_link_counters_get(struct sl_ctrl_link *ctrl_link, u32 counter, int *count);

int  sl_ctrl_link_cause_counters_init(struct sl_ctrl_link *ctrl_link);
void sl_ctrl_link_cause_counters_del(struct sl_ctrl_link *ctrl_link);
int  sl_ctrl_link_cause_counters_get(struct sl_ctrl_link *ctrl_link, u32 counter, int *count);

int  sl_ctrl_link_an_cause_counters_init(struct sl_ctrl_link *ctrl_link);
void sl_ctrl_link_an_cause_counters_del(struct sl_ctrl_link *ctrl_link);
int  sl_ctrl_link_an_cause_counters_get(struct sl_ctrl_link *ctrl_link, u32 counter, int *count);

void sl_ctrl_link_cause_counter_inc(struct sl_ctrl_link *ctrl_link, u64 cause_map);
void sl_ctrl_link_an_cause_counter_inc(struct sl_ctrl_link *ctrl_link, unsigned long cause_map);

#endif /* _SL_CTRL_LINK_COUNTERS_H_ */
