/******************************************************************************
 *
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#include <stdint.h>

#include <base/bt_mem_pool.h>
#include <osdep/os.h>

static os_mutex_t lock = OS_MUTEX_INITIALIZER;

static int create_mem_pool_list(struct bt_mem_pool *mpool)
{
	char *p;

	os_sem_init(&mpool->wait, 0, 1);

	/* blocks must be word aligned */
	CHECKIF(((mpool->info.block_size | (uintptr_t)mpool->buffer) & (sizeof(void *) - 1)) !=
		0U) {
		return -EINVAL;
	}

	mpool->free_list = NULL;
	p = mpool->buffer + mpool->info.block_size * (mpool->info.num_blocks - 1);

	while (p >= mpool->buffer) {
		*(char **)p = mpool->free_list;
		mpool->free_list = p;
		p -= mpool->info.block_size;
	}
	return 0;
}

static int mem_pool_list_init(void)
{
	int rc = 0;

	STRUCT_SECTION_FOREACH(bt_mem_pool, mpool) {
		rc = create_mem_pool_list(mpool);
		if (rc < 0) {
			goto out;
		}
	}

out:
	return rc;
}

int bt_mem_pool_init(struct bt_mem_pool *mpool, void *buffer, size_t block_size,
		     uint32_t num_blocks)
{
	int rc = 0;

	mpool->info.block_size = WB_UP(block_size);
	mpool->info.num_blocks = num_blocks;
	mpool->buffer = buffer;

	rc = create_mem_pool_list(mpool);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

int bt_mem_pool_alloc(struct bt_mem_pool *mpool, void **mem, os_timeout_t timeout)
{
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	int result;

	if (mpool->free_list != NULL) {
		/* take a free block */
		*mem = mpool->free_list;
		mpool->free_list = *(char **)(mpool->free_list);
		mpool->info.num_used++;
		result = 0;
	} else if (TIMEOUT_EQ(timeout, OS_TIMEOUT_NO_WAIT)) {
		/* don't wait for a free block to become available */
		*mem = NULL;
		result = -ENOMEM;
	} else {
		result = os_sem_take(&mpool->wait, timeout);
		if (result) {
			os_mutex_unlock(&lock);
			return result;
		}

		__ASSERT_NO_MSG(mpool->free_list != NULL);

		*mem = mpool->free_list;
		mpool->free_list = *(char **)(mpool->free_list);
		mpool->info.num_used++;
	}

	os_mutex_unlock(&lock);

	return result;
}

void bt_mem_pool_free(struct bt_mem_pool *mpool, void *mem)
{
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	*(char **)mem = mpool->free_list;
	mpool->free_list = (char *)mem;
	mpool->info.num_used--;

	(void)os_sem_give(&mpool->wait);

	os_mutex_unlock(&lock);
}

STACK_INIT(mem_pool_list_init, STACK_BASE_INIT, 0);