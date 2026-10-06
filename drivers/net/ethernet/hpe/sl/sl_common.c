// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "log/sl_log.h"
#include "sl_common.h"

#define SL_STR_ELLIPSIS_STR_SIZE 4
#define SL_STR_MAP_STR_SIZE_MIN 5

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_STR_LOG_NAME

#define SL_STR_UNKNOWN   "unknown"
#define SL_STR_NONE      "none"
#define SL_STR_TRUNCATED "..."
#define SL_STR_DELIMITER " "

int sl_str_map_create(u64 map, const struct sl_str_map *str_map,
		      char *dest, unsigned int dest_size)
{
	size_t        cat_len;
	const char   *item;
	unsigned int  bit;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "map create (map = 0x%016llX, dest_size = %u)", map, dest_size);

	if (!str_map) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "map create - NULL string map");
		return -EINVAL;
	}
	if (!str_map->strings) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "map create - NULL strings in map");
		return -EINVAL;
	}
	if (!dest) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "map create - NULL dest");
		return -EINVAL;
	}
	if (dest_size < SL_STR_MAP_STR_SIZE_MIN) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "map create - invalid string size (dest_size = %u)", dest_size);
		return -EINVAL;
	}

	dest[0] = '\0';
	for (bit = 0; bit < BITS_PER_TYPE(map); ++bit) {
		if (!(map & BIT_ULL(bit)))
			continue;

		item = (bit < str_map->num_strings && str_map->strings[bit]) ?
			str_map->strings[bit] : SL_STR_UNKNOWN;

		if (dest[0]) {
			cat_len = strlcat(dest, SL_STR_DELIMITER, dest_size);
			if (cat_len >= dest_size)
				goto truncated;
		}

		cat_len = strlcat(dest, item, dest_size);

		if (cat_len >= dest_size) {
			sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
				   "map create - string truncated (cat_len = %lu, dest_size = %u)",
				   cat_len, dest_size);

			goto truncated;
		}
	}

	if (!dest[0])
		strscpy(dest, SL_STR_NONE, dest_size);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "map create - string (map = 0x%016llX, str = %s)", map, dest);
	return 0;

truncated:
	strscpy(dest + dest_size - SL_STR_ELLIPSIS_STR_SIZE, SL_STR_TRUNCATED,
		SL_STR_ELLIPSIS_STR_SIZE);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "map create - string truncated (map = 0x%016llX, dest_size = %u, str = %s)",
		   map, dest_size, dest);
	return 0;
}
