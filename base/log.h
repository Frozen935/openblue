#ifndef __BASE_LOG_H__
#define __BASE_LOG_H__

#ifndef LOG_TAG
#define LOG_TAG "blue"
#endif

#define LOG_EN 1

/* Log level constants to satisfy switch/array usage */
enum stack_log_level {
	LOG_LEVEL_NONE = 0,
	LOG_LEVEL_ERR = 1,
	LOG_LEVEL_WRN = 2,
	LOG_LEVEL_INF = 3,
	LOG_LEVEL_DBG = 4
};

/* ===== Logging macros (safe no-op) ===== */

#ifndef CONFIG_STACK_LOG_LEVEL
#define CONFIG_STACK_LOG_LEVEL LOG_LEVEL_DBG
#endif

#if LOG_EN
#define _S_LINE(x)  #x
#define __S_LINE(x) _S_LINE(x)
#define __S_LINE__  __S_LINE(__LINE__)

extern void stack_log_hexdump(const void *data, size_t len);
extern bool stack_log_level_check(enum stack_log_level log_level);

#if defined(__linux__)
#define LOG_IMPL(level, fmt, ...)                                                                  \
	printf("[%d][%s:%d] " fmt "\n", level, LOG_TAG, __LINE__, ##__VA_ARGS__);

#elif defined(__NuttX__)
extern uint32_t stack_log_level_map(enum stack_log_level log_level);
#define LOG_IMPL(level, fmt, ...)                                                                  \
	syslog(stack_log_level_map(level),                                                         \
	       "[" LOG_TAG ":" __S_LINE__ "]"                                                      \
	       ": " fmt "\n",                                                                      \
	       ##args);
#else
#define LOG_IMPL(level, fmt, ...) /* no-op */
#endif

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_DBG
#define LOG_DBG(fmt, ...)                                                                          \
	do {                                                                                       \
		if (stack_log_level_check(LOG_LEVEL_DBG)) {                                        \
			LOG_IMPL(LOG_LEVEL_DBG, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_DBG(fmt, ...) /* no-op */
#endif

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_INF
#define LOG_INF(fmt, ...)                                                                          \
	do {                                                                                       \
		if (stack_log_level_check(LOG_LEVEL_INF)) {                                        \
			LOG_IMPL(LOG_LEVEL_INF, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_INF(fmt, ...) /* no-op */
#endif

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_WRN
#define LOG_WRN(fmt, ...)                                                                          \
	do {                                                                                       \
		if (stack_log_level_check(LOG_LEVEL_WRN)) {                                        \
			LOG_IMPL(LOG_LEVEL_WRN, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_WRN(fmt, ...) /* no-op */
#endif

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_ERR
#define LOG_ERR(fmt, ...)                                                                          \
	do {                                                                                       \
		if (stack_log_level_check(LOG_LEVEL_ERR)) {                                        \
			LOG_IMPL(LOG_LEVEL_ERR, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_ERR(fmt, ...) /* no-op */
#endif

#define LOG_HEXDUMP_DBG(buf, len, fmt, ...)                                                        \
	do {                                                                                       \
		if (stack_log_level_check(LOG_LEVEL_DBG)) {                                        \
			LOG_DBG(fmt, ##__VA_ARGS__);                                               \
			stack_log_hexdump(buf, len);                                               \
		}                                                                                  \
	} while (0)
#define LOG_HEXDUMP_INF(buf, len, fmt, ...)                                                        \
	do {                                                                                       \
		if (stack_log_level_check(LOG_LEVEL_INF)) {                                        \
			LOG_INF(fmt, ##__VA_ARGS__);                                               \
			stack_log_hexdump(buf, len);                                               \
		}                                                                                  \
	} while (0)
#define LOG_HEXDUMP_WRN(buf, len, fmt, ...)                                                        \
	do {                                                                                       \
		if (stack_log_level_check(LOG_LEVEL_WRN)) {                                        \
			LOG_WRN(fmt, ##__VA_ARGS__);                                               \
			stack_log_hexdump(buf, len);                                               \
		}                                                                                  \
	} while (0)

#else
#define LOG_DBG(fmt, ...)                   /* no-op */
#define LOG_INF(fmt, ...)                   /* no-op */
#define LOG_WRN(fmt, ...)                   /* no-op */
#define LOG_ERR(fmt, ...)                   /* no-op */
#define LOG_HEXDUMP_DBG(buf, len, fmt, ...) /* no-op */
#define LOG_HEXDUMP_INF(buf, len, fmt, ...) /* no-op */
#define LOG_HEXDUMP_WRN(buf, len, fmt, ...) /* no-op */
#endif                                      /* !LOG_EN */

#endif /* __BASE_LOG_H__ */