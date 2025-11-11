#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdbool.h>
#include <ctype.h>

#include "base/log.h"

static enum stack_log_level config_stack_log_level = CONFIG_STACK_LOG_LEVEL;

bool stack_log_level_check(enum stack_log_level log_level)
{
	if (config_stack_log_level < log_level) {
		return false;
	}

	return true;
}

void stack_log_hexdump(const void *data, size_t len)
{
	const unsigned char *p = data;
	char hex[16 * 3 + 1];
	char ascii[16 + 1];
	int i, hex_len = 0, ascii_len = 0;

	for (i = 0; i < len; i++) {
		int mod = i % 16;

		if (mod == 0) {
			hex_len = 0;
			ascii_len = 0;
		}

		hex_len += snprintf(hex + hex_len, sizeof(hex) - hex_len, "%02x ", p[i]);
		ascii[ascii_len++] = isprint(p[i]) ? p[i] : '.';

		if (mod == 15 || i == len - 1) {
			hex[hex_len] = '\0';
			ascii[ascii_len] = '\0';

#ifdef __linux__
			printf("%-48s %s\n", hex, ascii);
#elif defined(__NUTTX__)
			syslog(LOG_LEVEL_INF, "%-48s %s", hex, ascii);
#endif
		}
	}
}

#ifdef __NuttX__
uint32_t stack_log_level_map(enum stack_log_level log_level)
{
	switch (log_level) {
	case LOG_LEVEL_NONE:
		return 0;
	case LOG_LEVEL_ERR:
		return LOG_ERR;
	case LOG_LEVEL_WRN:
		return LOG_WARNING;
	case LOG_LEVEL_INF:
		return LOG_INFO;
	case LOG_LEVEL_DBG:
		return LOG_DEBUG;
	default:
		return 0;
	}
}
#endif