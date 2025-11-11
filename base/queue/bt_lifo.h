#ifndef __BASE_LIFO_H__
#define __BASE_LIFO_H__

#include <stdint.h>

#include <base/queue/bt_queue.h>

struct bt_lifo {
	struct bt_queue _queue;
};

#define BT_LIFO_INITIALIZER(obj)                                                                   \
	{                                                                                          \
		._queue = BT_QUEUE_INITIALIZER(obj._queue)                                         \
	}

#define bt_lifo_init(lifo) ({ bt_queue_init(&(lifo)->_queue); })

#define bt_lifo_put(lifo, data)                                                                    \
	({                                                                                         \
		void *_data = data;                                                                \
		bt_queue_prepend(&(lifo)->_queue, _data);                                          \
	})

#define bt_lifo_get(lifo, timeout)                                                                 \
	({                                                                                         \
		void *lg_ret = bt_queue_get(&(lifo)->_queue, timeout);                             \
		lg_ret;                                                                            \
	})

#endif /* __BASE_LIFO_H__ */