/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */


#if defined(CONFIG_BT_ASSERT_VERBOSE)
#define BT_ASSERT_PRINT(test)	      __ASSERT_NO_MSG(test)
#define BT_ASSERT_PRINT_MSG(fmt, ...) __ASSERT_PRINT(fmt, ##__VA_ARGS__)
#else
#define BT_ASSERT_PRINT(test)
#define BT_ASSERT_PRINT_MSG(fmt, ...)
#endif /* CONFIG_BT_ASSERT_VERBOSE */

#if defined(CONFIG_BT_ASSERT_PANIC)
#define BT_ASSERT_DIE()
#else
#define BT_ASSERT_DIE()
#endif /* CONFIG_BT_ASSERT_PANIC */

#if defined(CONFIG_BT_ASSERT)
#define BT_ASSERT(cond)                                                                            \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			BT_ASSERT_PRINT(cond);                                                     \
			BT_ASSERT_DIE();                                                           \
		}                                                                                  \
	} while (0)

#define BT_ASSERT_MSG(cond, fmt, ...)                                                              \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			BT_ASSERT_PRINT(cond);                                                     \
			BT_ASSERT_PRINT_MSG(fmt, ##__VA_ARGS__);                                   \
			BT_ASSERT_DIE();                                                           \
		}                                                                                  \
	} while (0)
#else
#define BT_ASSERT(cond)		      __ASSERT_NO_MSG(cond)
#define BT_ASSERT_MSG(cond, msg, ...) __ASSERT_MSG(cond, msg, ##__VA_ARGS__)
#endif /* CONFIG_BT_ASSERT*/
