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

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "bt_storage.h"

int bt_storage_name_steq(const char *name, const char *key, const char **next)
{
	return -ENOTSUP;
}

int bt_storage_load_subtree_direct(const char *subtree, bt_storage_load_direct_cb read_cb,
				   void *param)
{
	UNUSED(subtree);
	UNUSED(read_cb);
	UNUSED(param);
	return -ENOTSUP;
}

int bt_storage_name_next(const char *name, const char **next)
{
	UNUSED(name);
	if (next) {
		*next = NULL;
	}

	return -ENOTSUP;
}

int bt_storage_init(void)
{
	return -ENOTSUP;
}

int bt_storage_save_one(const char *key, const void *value, size_t len)
{
	UNUSED(key);
	UNUSED(value);
	UNUSED(len);
	return -ENOTSUP;
}

int bt_storage_load(void)
{
	return -ENOTSUP;
}

int settings_load(void)
{
	return bt_storage_load();
}

int bt_storage_delete(const char *key)
{
	UNUSED(key);
	return -ENOTSUP;
}
