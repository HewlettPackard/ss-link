/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _SL_COMMON_H_
#define _SL_COMMON_H_

#define is_flag_set(_flags, _flag) ({              \
		typeof(_flags) __flags = (_flags); \
		typeof(_flag)  __flag  = (_flag);  \
						   \
		((__flags & __flag) == __flag);    \
})

#endif /* _SL_COMMON_H_ */
