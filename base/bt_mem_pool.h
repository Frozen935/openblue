#ifndef __BASE_MEM_POOL_H__
#define __BASE_MEM_POOL_H__

#include <stdint.h>

#include <osdep/os.h>

struct bt_mem_pool_info {
	uint32_t num_blocks;
	size_t block_size;
	uint32_t num_used;
};

struct bt_mem_pool {
	os_sem_t wait;
	os_mutex_t lock;
	char *buffer;
	char *free_list;
	struct bt_mem_pool_info info;
};

#define BT_MEM_POOL_INITIALIZER(_pool, _pool_buffer, _pool_block_size, _pool_num_blocks)           \
	{                                                                                          \
		.lock = OS_MUTEX_INITIALIZER, .buffer = _pool_buffer, .free_list = NULL, .info = { \
			_pool_num_blocks,                                                          \
			_pool_block_size,                                                          \
			0                                                                          \
		}                                                                                  \
	}

#define BT_MEM_POOL_DEFINE_IN_SECT(name, pool_block_size, pool_num_blocks, pool_align)             \
	BUILD_ASSERT(((pool_block_size) % (pool_align)) == 0,                                      \
		     "pool_block_size must be a multiple of pool_align");                          \
	BUILD_ASSERT((((pool_align) & ((pool_align) - 1)) == 0),                                   \
		     "pool_align must be a power of 2");                                           \
	char __aligned(WB_UP(                                                                      \
		pool_align)) _bt_mem_pool_buf_##name[(pool_num_blocks) * WB_UP(pool_block_size)];  \
	STRUCT_SECTION_ITERABLE(bt_mem_pool, name) = BT_MEM_POOL_INITIALIZER(                      \
		name, _bt_mem_pool_buf_##name, WB_UP(pool_block_size), pool_num_blocks)

#define BT_MEM_POOL_DEFINE(name, pool_block_size, pool_num_blocks, pool_align)                     \
	BT_MEM_POOL_DEFINE_IN_SECT(name, pool_block_size, pool_num_blocks, pool_align)

#define BT_MEM_POOL_DEFINE_IN_SECT_STATIC(name, pool_block_size, pool_num_blocks, pool_align)      \
	BUILD_ASSERT(((pool_block_size) % (pool_align)) == 0,                                      \
		     "pool_block_size must be a multiple of pool_align");                          \
	BUILD_ASSERT((((pool_align) & ((pool_align) - 1)) == 0),                                   \
		     "pool_align must be a power of 2");                                           \
	char __aligned(WB_UP(                                                                      \
		pool_align)) _bt_mem_pool_buf_##name[(pool_num_blocks) * WB_UP(pool_block_size)];  \
	STRUCT_SECTION_ITERABLE(bt_mem_pool, name) = BT_MEM_POOL_INITIALIZER(                      \
		name, _bt_mem_pool_buf_##name, WB_UP(pool_block_size), pool_num_blocks)

#define BT_MEM_POOL_DEFINE_STATIC(name, pool_block_size, pool_num_blocks, pool_align)              \
	BT_MEM_POOL_DEFINE_IN_SECT_STATIC(name, pool_block_size, pool_num_blocks, pool_align)

int bt_mem_pool_init(struct bt_mem_pool *mpool, void *buffer, size_t block_size,
		     uint32_t num_blocks);
int bt_mem_pool_alloc(struct bt_mem_pool *mpool, void **mem, os_timeout_t timeout);
void bt_mem_pool_free(struct bt_mem_pool *mpool, void *mem);

#endif /* __BASE_MEM_POOL_H__ */