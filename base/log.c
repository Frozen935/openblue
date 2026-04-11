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

#include <stdio.h>

#include <stdint.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdbool.h>
#include <ctype.h>

#include "base/log.h"

static enum stack_log_level config_stack_log_level = CONFIG_STACK_LOG_LEVEL;

bool bt_log_level_check(enum stack_log_level log_level)
{
	if (config_stack_log_level < log_level) {
		return false;
	}

	return true;
}

#ifdef __NuttX__
uint32_t bt_log_level_map(enum stack_log_level log_level)
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
