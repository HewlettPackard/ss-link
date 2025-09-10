/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_JACK_NIC2_EMU_H_
#define _SL_MEDIA_DATA_JACK_NIC2_EMU_H_

struct sl_media_lgrp;

int  sl_media_data_jack_lgrp_connect(struct sl_media_lgrp *media_lgrp);
void sl_media_data_jack_unregister_event_notifier(void);

#endif /* _SL_MEDIA_DATA_JACK_NIC2_EMU_H_ */
