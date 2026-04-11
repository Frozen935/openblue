#include <stdint.h>
#include <string.h>
#include <setjmp.h>

#if defined(__has_include)
#if __has_include(<cmocka.h>)
#include <cmocka.h>
#else
#include <cmocka.h>
#endif
#else
#include <cmocka.h>
#endif

#include <base/bt_poll.h>
#include <base/queue/bt_queue.h>
#include <osdep/os.h>

struct queue_item {
	bt_snode_t node;
	int value;
};

struct signal_thread_arg {
	struct bt_poll_signal *sig;
	int result;
	uint32_t delay_ms;
};

static void signal_thread(void *arg)
{
	struct signal_thread_arg *ctx = (struct signal_thread_arg *)arg;

	os_sleep_ms(ctx->delay_ms);
	assert_int_equal(bt_poll_signal_raise(ctx->sig, ctx->result), 0);
}

struct queue_thread_arg {
	struct bt_queue *queue;
	struct queue_item *item;
	uint32_t delay_ms;
};

static void queue_thread(void *arg)
{
	struct queue_thread_arg *ctx = (struct queue_thread_arg *)arg;

	os_sleep_ms(ctx->delay_ms);
	bt_queue_append(ctx->queue, ctx->item);
	bt_poll_handle_obj_events(&ctx->queue->poll_events, BT_POLL_STATE_DATA_AVAILABLE);
}

static void test_bt_poll_signal_nowait_ready(void **state)
{
	(void)state;
	struct bt_poll_signal sig = BT_POLL_SIGNAL_INITIALIZER(sig);
	struct bt_poll_event event =
		BT_POLL_EVENT_INITIALIZER(BT_POLL_TYPE_SIGNAL, BT_POLL_MODE_NOTIFY_ONLY, &sig);

	assert_int_equal(bt_poll_signal_raise(&sig, 123), 0);
	assert_int_equal(bt_poll(&event, 1, OS_TIMEOUT_NO_WAIT), 0);
	assert_true((event.state & BT_POLL_STATE_SIGNALED) != 0U);
	assert_int_equal(sig.result, 123);
}

static void test_bt_poll_signal_wait_wakeup(void **state)
{
	(void)state;
	struct bt_poll_signal sig = BT_POLL_SIGNAL_INITIALIZER(sig);
	struct bt_poll_event event =
		BT_POLL_EVENT_INITIALIZER(BT_POLL_TYPE_SIGNAL, BT_POLL_MODE_NOTIFY_ONLY, &sig);
	struct signal_thread_arg arg = {
		.sig = &sig,
		.result = 456,
		.delay_ms = 20,
	};
	os_thread_t th;

	assert_int_equal(
		os_thread_create(&th, signal_thread, &arg, "poll_sig", OS_PRIORITY(0), 0), 0);
	assert_int_equal(bt_poll(&event, 1, OS_SECONDS(1)), 0);
	assert_true((event.state & BT_POLL_STATE_SIGNALED) != 0U);
	assert_int_equal(sig.result, 456);
	assert_int_equal(os_thread_join(&th, OS_TIMEOUT_FOREVER), 0);
}

static void test_bt_poll_queue_data_available(void **state)
{
	(void)state;
	struct bt_queue queue;
	struct bt_poll_event event;
	struct queue_item item = {
		.value = 7,
	};
	struct queue_thread_arg arg = {
		.queue = &queue,
		.item = &item,
		.delay_ms = 20,
	};
	os_thread_t th;

	bt_queue_init(&queue);
	bt_poll_event_init(&event, BT_POLL_TYPE_DATA_AVAILABLE, BT_POLL_MODE_NOTIFY_ONLY, &queue);

	assert_int_equal(
		os_thread_create(&th, queue_thread, &arg, "poll_q", OS_PRIORITY(0), 0), 0);
	assert_int_equal(bt_poll(&event, 1, OS_SECONDS(1)), 0);
	assert_true((event.state & BT_POLL_STATE_DATA_AVAILABLE) != 0U);

	struct queue_item *out = (struct queue_item *)bt_queue_get(&queue, OS_TIMEOUT_NO_WAIT);
	assert_non_null(out);
	assert_ptr_equal(out, &item);
	assert_int_equal(out->value, 7);
	assert_int_equal(os_thread_join(&th, OS_TIMEOUT_FOREVER), 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bt_poll_signal_nowait_ready),
		cmocka_unit_test(test_bt_poll_signal_wait_wakeup),
		cmocka_unit_test(test_bt_poll_queue_data_available),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
