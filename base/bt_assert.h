/*
 * Copyright (C) 2026
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BASE_ASSERT_H__
#define __BASE_ASSERT_H__

#include <assert.h>
#include <stdlib.h>
#include <base/log.h>

/*
#ifdef NDEBUG
#define ASSERT_EN 0
#else
#define ASSERT_EN 1
#endif
*/

#define ASSERT_EN 1

#if ASSERT_EN
#ifndef assert
static inline void __bt_assert_fail(const char *expr, const char *file, int line)
{
	LOG_ERR("Assertion failed: %s at %s:%d", expr, file, line);
	abort();
}
#else
#define __bt_assert_fail(expr, file, line) assert(0)
#endif

/* ===== Assert macros ===== */
#define __ASSERT_NO_MSG(test)                                                                      \
	do {                                                                                       \
		if (!(test)) {                                                                     \
			__bt_assert_fail(#test, __FILE__, __LINE__);                               \
		}                                                                                  \
	} while (0)

#define __ASSERT_MSG(test, msg, ...)                                                               \
	do {                                                                                       \
		if (!(test)) {                                                                     \
			LOG_ERR(msg, ##__VA_ARGS__);                                               \
			__bt_assert_fail(#test, __FILE__, __LINE__);                               \
		}                                                                                  \
	} while (0)

#define __ASSERT(test, msg, ...) __ASSERT_MSG(test, msg, ##__VA_ARGS__)

#define __ASSERT_PRINT(fmt, ...)                                                                   \
	do {                                                                                       \
		LOG_INF(fmt, ##__VA_ARGS__);                                                       \
		__bt_assert_fail(NULL, __FILE__, __LINE__);                                        \
	} while (0)

#else
#define __ASSERT_NO_MSG(cond)
#define __ASSERT_MSG(cond, msg, ...)
#define __ASSERT(cond, msg, ...)
#define __ASSERT_PRINT(...)
#endif
#endif /* __BASE_ASSERT_H__ */
