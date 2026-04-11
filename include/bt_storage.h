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

#ifndef __INCLUDE_BT_STORAGE_H__
#define __INCLUDE_BT_STORAGE_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BT_STORAGE_HANDLER_DEFINE(name, subtree, get, set, commit, export) /* no-op */
#define BT_STORAGE_HANDLER_DEFINE_WITH_CPRIO(name, subtree, get, set, commit, export,              \
					     cprio) /* no-op */

typedef ssize_t (*bt_storage_read_cb)(void *cb_arg, void *data, size_t len);

typedef int (*bt_storage_load_direct_cb)(const char *key, size_t len, bt_storage_read_cb read_cb,
					 void *cb_arg, void *param);

typedef struct {
	int (*storage_init)(void);
	int (*storage_load)(void);
	int (*storage_read)(const char *key, void *data, size_t len);
	int (*storage_write)(const char *key, const void *data, size_t len);
	int (*storage_delete)(const char *key);
} bt_storage_backend_t;

int bt_storage_name_steq(const char *name, const char *key, const char **next);
int bt_storage_load_subtree_direct(const char *subtree, bt_storage_load_direct_cb read_cb,
				   void *param);
int bt_storage_name_next(const char *name, const char **next);
int bt_storage_init(void);
int bt_storage_save_one(const char *key, const void *value, size_t len);
int bt_storage_load(void);
int bt_storage_delete(const char *key);

#endif
