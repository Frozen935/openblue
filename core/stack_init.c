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

/* Explicit STACK_INIT registry traversal. */
#include <stdbool.h>
#include <stddef.h>

#include "bt_stack_init.h"

extern int bt_mem_pool_registry_init(void);
extern int bt_work_main_work_init(void);
extern int bt_driver_userchan_init(void);
extern int bt_driver_h4_init(void);

#if defined(CONFIG_BT_LONG_WQ)
extern int bt_long_wq_init(void);
#endif

#if defined(CONFIG_BT_MONITOR)
extern int bt_monitor_stack_init(void);
#endif

#if defined(CONFIG_BT_CONN_TX_NOTIFY_WQ)
extern int bt_conn_tx_workq_stack_init(void);
#endif

#if defined(CONFIG_BT_TX_PROCESSOR_THREAD)
extern int bt_tx_processor_stack_init(void);
#endif

static const struct stack_init_entry stack_init_base_entries[] = {
	{bt_mem_pool_registry_init, 0U, STACK_BASE_INIT, "bt_mem_pool_registry_init"},
	{bt_work_main_work_init, 1U, STACK_BASE_INIT, "bt_work_main_work_init"},
};

#if defined(CONFIG_OPENBLUE_BT_DRIVER_TYPE_USERCHAN) || defined(CONFIG_OPENBLUE_BT_DRIVER_TYPE_H4)
static const struct stack_init_entry stack_init_driver_entries[] = {
#if defined(CONFIG_OPENBLUE_BT_DRIVER_TYPE_USERCHAN)
	{bt_driver_userchan_init, 0U, STACK_BASE_INIT, "bt_driver_userchan_init"},
#endif
#if defined(CONFIG_OPENBLUE_BT_DRIVER_TYPE_H4)
	{bt_driver_h4_init, 0U, STACK_BASE_INIT, "bt_driver_h4_init"},
#endif
};
#define STACK_INIT_DRIVER_ENTRY_COUNT ARRAY_SIZE(stack_init_driver_entries)
#else
static const struct stack_init_entry *const stack_init_driver_entries = NULL;
#define STACK_INIT_DRIVER_ENTRY_COUNT 0U
#endif

#if defined(CONFIG_BT_LONG_WQ) || defined(CONFIG_BT_CONN_TX_NOTIFY_WQ) ||                          \
	defined(CONFIG_BT_MONITOR) || defined(CONFIG_BT_TX_PROCESSOR_THREAD)
static const struct stack_init_entry stack_init_host_entries[] = {
#if defined(CONFIG_BT_LONG_WQ)
	{bt_long_wq_init, CONFIG_BT_LONG_WQ_INIT_PRIO, STACK_BASE_INIT, "bt_long_wq_init"},
#endif
#if defined(CONFIG_BT_CONN_TX_NOTIFY_WQ)
	{bt_conn_tx_workq_stack_init, CONFIG_BT_CONN_TX_NOTIFY_WQ_INIT_PRIORITY, STACK_BASE_INIT,
	 "bt_conn_tx_workq_stack_init"},
#endif
#if defined(CONFIG_BT_MONITOR)
	{bt_monitor_stack_init, 60U, STACK_BASE_INIT, "bt_monitor_stack_init"},
#endif
#if defined(CONFIG_BT_TX_PROCESSOR_THREAD)
	{bt_tx_processor_stack_init, 999U, STACK_BASE_INIT, "bt_tx_processor_stack_init"},
#endif
};
#define STACK_INIT_HOST_ENTRY_COUNT ARRAY_SIZE(stack_init_host_entries)
#else
static const struct stack_init_entry *const stack_init_host_entries = NULL;
#define STACK_INIT_HOST_ENTRY_COUNT 0U
#endif

static int bt_stack_run_entries(const struct stack_init_entry *entries, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		int err = entries[i].init();

		if (err != 0) {
			return err;
		}
	}

	return 0;
}

int bt_stack_init_once(void)
{
	static bool initialized;
	int err;

	if (initialized) {
		return 0;
	}

	err = bt_stack_run_entries(stack_init_base_entries, ARRAY_SIZE(stack_init_base_entries));
	if (err != 0) {
		return err;
	}

	err = bt_stack_run_entries(stack_init_driver_entries, STACK_INIT_DRIVER_ENTRY_COUNT);
	if (err != 0) {
		return err;
	}

	err = bt_stack_run_entries(stack_init_host_entries, STACK_INIT_HOST_ENTRY_COUNT);
	if (err != 0) {
		return err;
	}

	initialized = true;

	return 0;
}
