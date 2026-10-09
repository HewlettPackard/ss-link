/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _SL_COMMON_H_
#define _SL_COMMON_H_

#include <linux/types.h>

#define SL_STR_MAP_STR_SIZE_DEFAULT 650 /* need headroom for frame size of 1024 */

#define is_flag_set(_flags, _flag) ({              \
		typeof(_flags) __flags = (_flags); \
		typeof(_flag)  __flag  = (_flag);  \
						   \
		((__flags & __flag) == __flag);    \
})

struct sl_str_map {
	const char * const *strings;
	unsigned int        num_strings;
};

int sl_str_map_create(u64 map, const struct sl_str_map *str_map,
		      char *dest, unsigned int dest_size);

#endif /* _SL_COMMON_H_ */
