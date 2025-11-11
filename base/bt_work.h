#ifndef __BASE_WORK_H__
#define __BASE_WORK_H__

#include <stdint.h>

#include <base/utils.h>
#include <osdep/os.h>

#include <utils/bt_slist.h>

struct bt_work_delayable;
struct bt_work_sync;
struct bt_work_q;
struct bt_work;

typedef void (*bt_work_handler_t)(struct bt_work *work);

enum {

	BT_WORK_RUNNING_BIT = 0,
	BT_WORK_CANCELING_BIT = 1,
	BT_WORK_QUEUED_BIT = 2,
	BT_WORK_DELAYED_BIT = 3,
	BT_WORK_FLUSHING_BIT = 4,

	BT_WORK_MASK = BIT(BT_WORK_DELAYED_BIT) | BIT(BT_WORK_QUEUED_BIT) |
		       BIT(BT_WORK_RUNNING_BIT) | BIT(BT_WORK_CANCELING_BIT) |
		       BIT(BT_WORK_FLUSHING_BIT),

	/* Static work flags */
	BT_WORK_DELAYABLE_BIT = 8,
	BT_WORK_DELAYABLE = BIT(BT_WORK_DELAYABLE_BIT),

	/* Dynamic work queue flags */
	BT_WORK_QUEUE_STARTED_BIT = 0,
	BT_WORK_QUEUE_STARTED = BIT(BT_WORK_QUEUE_STARTED_BIT),
	BT_WORK_QUEUE_BUSY_BIT = 1,
	BT_WORK_QUEUE_BUSY = BIT(BT_WORK_QUEUE_BUSY_BIT),
	BT_WORK_QUEUE_DRAIN_BIT = 2,
	BT_WORK_QUEUE_DRAIN = BIT(BT_WORK_QUEUE_DRAIN_BIT),
	BT_WORK_QUEUE_PLUGGED_BIT = 3,
	BT_WORK_QUEUE_PLUGGED = BIT(BT_WORK_QUEUE_PLUGGED_BIT),
	BT_WORK_QUEUE_STOP_BIT = 4,
	BT_WORK_QUEUE_STOP = BIT(BT_WORK_QUEUE_STOP_BIT),

	/* Static work queue flags */
	BT_WORK_QUEUE_NO_YIELD_BIT = 8,
	BT_WORK_QUEUE_NO_YIELD = BIT(BT_WORK_QUEUE_NO_YIELD_BIT),
	BT_WORK_RUNNING = BIT(BT_WORK_RUNNING_BIT),
	BT_WORK_CANCELING = BIT(BT_WORK_CANCELING_BIT),
	BT_WORK_QUEUED = BIT(BT_WORK_QUEUED_BIT),
	BT_WORK_DELAYED = BIT(BT_WORK_DELAYED_BIT),
	BT_WORK_FLUSHING = BIT(BT_WORK_FLUSHING_BIT),
};

/** @brief A structure used to submit work. */
struct bt_work {

	/* Node to link into bt_work_q pending list. */
	bt_snode_t node;

	/* The function to be invoked by the work queue thread. */
	bt_work_handler_t handler;

	/* The queue on which the work item was last submitted. */
	struct bt_work_q *queue;
	uint32_t flags;
};

#define BT_WORK_INITIALIZER(work_handler)                                                           \
	{                                                                                          \
		.handler = (work_handler),                                                         \
	}

/** @brief A structure used to submit work after a delay. */
struct bt_work_delayable {
	/* The work item. */
	struct bt_work work;

	/* Timeout used to submit work after a delay. */
	os_timer_t timeout;

	/* The queue to which the work should be submitted. */
	struct bt_work_q *queue;
};

#define BT_WORK_DELAYABLE_INITIALIZER(work_handler)                                                \
	{                                                                                          \
		.work = {                                                                          \
			.handler = (work_handler),                                                 \
			.flags = BT_WORK_DELAYABLE,                                                \
		},                                                                                 \
	}

#define BT_WORK_DELAYABLE_DEFINE(work, work_handler)                                               \
	struct bt_work_delayable work = BT_WORK_DELAYABLE_INITIALIZER(work_handler)

struct bt_work_flusher {
	struct bt_work work;
	os_sem_t sem;
};

struct bt_work_canceller {
	bt_snode_t node;
	struct bt_work *work;
	os_sem_t sem;
};

struct bt_work_sync {
	union {
		struct bt_work_flusher flusher;
		struct bt_work_canceller canceller;
	};
};

struct bt_work_queue_config {

	const char *name;

	bool no_yield;

	/** Control whether the work queue thread should be marked as
	 * essential thread.
	 */
	bool essential;

	uint32_t work_timeout_ms;
};

/** @brief A structure used to hold work until it can be processed. */
struct bt_work_q {
	/* The thread that animates the work. */
	os_thread_t thread;

	/* The thread ID that animates the work. This may be an external thread
	 * if bt_work_queue_run() is used.
	 */
	os_tid_t thread_id;

	/* All the following fields must be accessed only while the
	 * work module spinlock is held.
	 */

	/* List of bt_work items to be worked. */
	bt_slist_t pending;

	/* Wait queue for idle work thread. */
	os_sem_t notifyq;

	/* Wait queue for threads waiting for the queue to drain. */
	os_sem_t drainq;

	/* Flags describing queue state. */
	uint32_t flags;
};

void bt_work_init(struct bt_work *work, bt_work_handler_t handler);
int bt_work_busy_get(const struct bt_work *work);

int bt_work_submit_to_queue(struct bt_work_q *queue, struct bt_work *work);
int bt_work_submit(struct bt_work *work);
bool bt_work_flush(struct bt_work *work, struct bt_work_sync *sync);
int bt_work_cancel(struct bt_work *work);
bool bt_work_cancel_sync(struct bt_work *work, struct bt_work_sync *sync);
void bt_work_queue_init(struct bt_work_q *queue);
void bt_work_queue_start(struct bt_work_q *queue, size_t stack_size, int prio,
			  const struct bt_work_queue_config *cfg);
void bt_work_queue_run(struct bt_work_q *queue, const struct bt_work_queue_config *cfg);
int bt_work_queue_drain(struct bt_work_q *queue, bool plug);
int bt_work_queue_unplug(struct bt_work_q *queue);
int bt_work_queue_stop(struct bt_work_q *queue, os_timeout_t timeout);
void bt_work_init_delayable(struct bt_work_delayable *dwork, bt_work_handler_t handler);
int bt_work_delayable_busy_get(const struct bt_work_delayable *dwork);
int bt_work_schedule_for_queue(struct bt_work_q *queue, struct bt_work_delayable *dwork,
			       os_timeout_t delay);
int bt_work_schedule(struct bt_work_delayable *dwork, os_timeout_t delay);
int bt_work_reschedule_for_queue(struct bt_work_q *queue, struct bt_work_delayable *dwork,
				 os_timeout_t delay);
int bt_work_reschedule(struct bt_work_delayable *dwork, os_timeout_t delay);
bool bt_work_flush_delayable(struct bt_work_delayable *dwork, struct bt_work_sync *sync);

int bt_work_cancel_delayable(struct bt_work_delayable *dwork);
bool bt_work_cancel_delayable_sync(struct bt_work_delayable *dwork, struct bt_work_sync *sync);

/* Provide the implementation for inline functions declared above */
struct bt_work_q *bt_work_main_work_queue(void);

static inline bool bt_work_is_pending(const struct bt_work *work)
{
	return bt_work_busy_get(work) != 0;
}

static inline struct bt_work_delayable *bt_work_delayable_from_work(struct bt_work *work)
{
	return CONTAINER_OF(work, struct bt_work_delayable, work);
}

static inline bool bt_work_delayable_is_pending(const struct bt_work_delayable *dwork)
{
	return bt_work_delayable_busy_get(dwork) != 0;
}

static inline uint32_t bt_work_delayable_remaining_get(const struct bt_work_delayable *dwork)
{
	return os_timer_remaining_ms(&dwork->timeout);
}

static inline os_tid_t bt_work_queue_thread_get(struct bt_work_q *queue)
{
	return queue->thread_id;
}

#define BT_WORK_DEFINE(work, work_handler) struct bt_work work = BT_WORK_INITIALIZER(work_handler)

#endif /* __BASE_WORK_H__ */