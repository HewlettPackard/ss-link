/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_LOG_H_
#define _SL_LOG_H_

#include <linux/kern_levels.h>
#include <linux/dynamic_debug.h>

#define SL_LOG_CONNECT_ID_LEN 12

#define SL_LOG_BLOCK             "sl"
#define SL_LOG_LDEV_LOG_NAME     "ldev"
#define SL_LOG_LGRP_LOG_NAME     "lgrp"
#define SL_LOG_LINK_LOG_NAME     "link"
#define SL_LOG_FEC_LOG_NAME      "fec"
#define SL_LOG_MAC_LOG_NAME      "mac"
#define SL_LOG_LLR_LOG_NAME      "llr"
#define SL_LOG_MEDIA_LOG_NAME    "media"
#define SL_LOG_SYSFS_LOG_NAME    "sysfs"
#define SL_LOG_DEBUGFS_LOG_NAME  "debugfs"
#define SL_LOG_TEST_LOG_NAME     "test"

void sl_log(void *ptr, const char *level, const char *block,
	const char *name, const char *text, ...) __printf(5, 6);
void sl_log_err_trace(void *ptr, const char *block,
	const char *name, const char *text, ...) __printf(4, 5);
void sl_log_warn_trace(void *ptr, const char *block,
	const char *name, const char *text, ...) __printf(4, 5);

#if defined(CONFIG_DYNAMIC_DEBUG)

void __sl_log_dynamic_dbg(struct _ddebug *desc, void *ptr,
	const char *block, const char *name, const char *text, ...) __printf(5, 6);

#define __sl_log_dynamic_func_call(_id, _ptr, _block, _name, _text, _func, ...) \
	do {                                                                    \
		DEFINE_DYNAMIC_DEBUG_METADATA(_id, _text);                      \
		if (DYNAMIC_DEBUG_BRANCH(_id))                                  \
			_func(&_id, _ptr, _block, _name, _text, ##__VA_ARGS__); \
	} while (0)

#define _sl_log_dynamic_func_call(_ptr, _block, _name, _text, _func, ...)    \
	__sl_log_dynamic_func_call(__UNIQUE_ID(ddebug), _ptr, _block, _name, \
	_text, _func, ##__VA_ARGS__)

#define sl_log_dynamic_dbg(_ptr, _block, _name, _text, ...)   \
	_sl_log_dynamic_func_call(_ptr, _block, _name, _text, \
	__sl_log_dynamic_dbg, ##__VA_ARGS__)

#define sl_log_dbg(_ptr, _block, _name, _text, ...) \
	sl_log_dynamic_dbg((_ptr), (_block), (_name), (_text), ##__VA_ARGS__)

#else

#define sl_log_dbg(_ptr, _block, _name, _text, ...) \
	sl_log((_ptr), KERN_DEBUG, (_block), (_name), (_text), ##__VA_ARGS__)

#endif

#define sl_log_info(_ptr, _block, _name, _text, ...) \
	sl_log((_ptr), KERN_INFO, (_block), (_name), (_text), ##__VA_ARGS__)
#define sl_log_warn(_ptr, _block, _name, _text, ...) \
	sl_log((_ptr), KERN_WARNING, (_block), (_name), (_text), ##__VA_ARGS__)
#define sl_log_err(_ptr, _block, _name, _text, ...) \
	sl_log((_ptr), KERN_ERR, (_block), (_name), (_text), ##__VA_ARGS__)

#endif /* _SL_LOG_H_ */
