#ifndef __BASE_ASSERT_H__
#define __BASE_ASSERT_H__

#include <assert.h>
#include <stdlib.h>
#include <base/log.h>

#define ASSERT_EN 1

static inline void __bt_assert_fail(const char *expr, const char *file, int line)
{
#ifdef NDEBUG
	LOG_ERR("Assertion failed: %s at %s:%d", expr, file, line);
	abort();
#else
	assert(0);
#endif
}

#if ASSERT_EN
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

#define __ASSERT_PRINT(fmt, ...)                                                                   \
	do {                                                                                       \
		LOG_INF(fmt, ##__VA_ARGS__);                                                       \
		__bt_assert_fail(NULL, __FILE__, __LINE__);                                        \
	} while (0)

#else
#define __ASSERT_NO_MSG(cond)
#define __ASSERT_MSG(cond, msg, ...)
#define __ASSERT_PRINT(...)
#endif
#endif /* __BASE_ASSERT_H__ */