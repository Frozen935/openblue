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

#ifndef __INCLUDE_BT_STACK_INIT_H__
#define __INCLUDE_BT_STACK_INIT_H__

#include <stdint.h>

#include <bt_toolchain_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STACK_BASE_INIT
#define STACK_BASE_INIT 1
#endif
#ifndef STACK_RUN_INIT
#define STACK_RUN_INIT 2
#endif
#ifndef STACK_SVC_INIT
#define STACK_SVC_INIT 3
#endif

#ifndef BT_STACK_KERNEL_INIT_PRIORITY_DEFAULT
#define BT_STACK_KERNEL_INIT_PRIORITY_DEFAULT 40
#endif

#ifndef BT_STACK_APPLICATION_INIT_PRIORITY
#define BT_STACK_APPLICATION_INIT_PRIORITY 90
#endif

/* Function prototype for init callbacks */
typedef int (*stack_init_fn_t)(void);

struct stack_init_entry {
	stack_init_fn_t init; /* initializer function */
	uint16_t prio;        /* priority within level (ascending) */
	uint16_t level;       /* broad level ordering (ascending) */
	const char *name;     /* optional name for diagnostics */
};

/*
 * Transitional compatibility macro.
 *
 * OpenBlue no longer relies on linker-section based init aggregation. `STACK_INIT`
 * is kept only as local metadata so call sites do not need to be rewritten all at
 * once while the runtime uses an explicit init table from `core/stack_init.c`.
 */
#define STACK_INIT(fn, level, prio)                                                                \
	static const struct stack_init_entry __maybe_unused __stack_init_entry_##fn = {            \
		(fn), (uint16_t)(prio), (uint16_t)(level), #fn}

/* Public API: run all registered initializers */
int bt_stack_init_once(void);

#ifdef __cplusplus
}
#endif

#endif /* __STACK_INIT_H__ */
