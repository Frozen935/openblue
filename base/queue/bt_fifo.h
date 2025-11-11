#ifndef __BASE_FIFO_H__
#define __BASE_FIFO_H__

#include <stdint.h>

#include <base/queue/bt_queue.h>

struct bt_fifo {
	struct bt_queue _queue;
};

#define BT_FIFO_INITIALIZER(obj)                                                                   \
	{                                                                                          \
		._queue = BT_QUEUE_INITIALIZER(obj._queue)                                         \
	}

#define BT_FIFO_DEFINE(name) struct bt_fifo name = BT_FIFO_INITIALIZER(name)

#define bt_fifo_init(fifo) ({ bt_queue_init(&(fifo)->_queue); })

#define bt_fifo_cancel_wait(fifo) ({ bt_queue_cancel_wait(&(fifo)->_queue); })

#define bt_fifo_put(fifo, data) ({ bt_queue_append(&(fifo)->_queue, data); })

#define bt_fifo_get(fifo, timeout) ({ bt_queue_get(&(fifo)->_queue, timeout); })

#define bt_fifo_is_empty(fifo) bt_queue_is_empty(&(fifo)->_queue)

#define bt_fifo_peek_head(fifo) ({ bt_queue_peek_head(&(fifo)->_queue); })

#define bt_fifo_peek_tail(fifo) ({ bt_queue_peek_tail(&(fifo)->_queue); })

#endif /* __BASE_FIFO_H__ */