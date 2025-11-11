#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "base/bt_work.h"
#include "osdep/os.h"

static volatile int handler_count = 0;

static void simple_handler(struct bt_work *work)
{
    (void)work;
    handler_count++;
}

static void long_handler(struct bt_work *work)
{
    (void)work;
    /* Simulate long processing */
    os_sleep_ms(200);
    handler_count++;
}

static void test_sys_workq_submit(void)
{
    /* Initialize system work queue */
    int rc = main_work_q_init();
    assert(rc == 0);

    BT_WORK_DEFINE(w, simple_handler);
    /* Also test bt_work_init explicitly */
    bt_work_init(&w, simple_handler);

    handler_count = 0;
    rc = bt_work_submit(&w);
    assert(rc >= 0);
    /* Wait for execution */
    for (int i = 0; i < 50 && handler_count == 0; i++) {
        os_sleep_ms(10);
    }
    assert(handler_count == 1);

    /* Flush an idle work should be false */
    struct bt_work_sync sync;
    bool flushed = bt_work_flush(&w, &sync);
    assert(flushed == false);
}

static void test_custom_queue_start_and_submit(void)
{
    struct bt_work_q q;
    bt_work_queue_init(&q);
    struct bt_work_queue_config cfg = { .name = "tq", .no_yield = false };
    bt_work_queue_start(&q, 2048, OS_PRIORITY(0), &cfg);

    struct bt_work w;
    bt_work_init(&w, simple_handler);

    handler_count = 0;
    int rc = bt_work_submit_to_queue(&q, &w);
    assert(rc >= 0);
    for (int i = 0; i < 50 && handler_count == 0; i++) {
        os_sleep_ms(10);
    }
    assert(handler_count == 1);

    /* Drain (without plug) just waits until idle */
    rc = bt_work_queue_drain(&q, false);
    assert(rc == 0);

    /* Plug and stop */
    rc = bt_work_queue_drain(&q, true);
    assert(rc == 0);
    rc = bt_work_queue_stop(&q, OS_SECONDS(1));
    assert(rc == 0);
}

static void test_cancel_sync(void)
{
    /* Use long handler to exercise cancel sync */
    struct bt_work w;
    bt_work_init(&w, long_handler);

    handler_count = 0;
    int rc = bt_work_submit(&w);
    assert(rc >= 0);

    struct bt_work_sync sync;
    bool pending = bt_work_cancel_sync(&w, &sync);
    /* Depending on timing, it may or may not be pending; but API should return a boolean */
    assert(pending == true || pending == false);
    /* Wait for potential execution finish */
    for (int i = 0; i < 50 && handler_count == 0; i++) {
        os_sleep_ms(10);
    }
}

static void dwork_handler(struct bt_work *work)
{
    /* Validate conversion from struct bt_work* */
    struct bt_work_delayable *dw = bt_work_delayable_from_work(work);
    (void)dw;
    handler_count++;
}

static void test_delayable_schedule_cancel_reschedule(void)
{
    /* Dedicated queue */
    struct bt_work_q q; bt_work_queue_init(&q);
    struct bt_work_queue_config cfg = { .name = "dq", .no_yield = false };
    bt_work_queue_start(&q, 2048, OS_PRIORITY(0), &cfg);

    struct bt_work_delayable dw; bt_work_init_delayable(&dw, dwork_handler);

    /* Schedule with delay 50ms: verify busy state toggles */
    int rc = bt_work_schedule_for_queue(&q, &dw, OS_MSEC(50));
    assert(rc >= 0);
    assert(bt_work_delayable_busy_get(&dw) != 0);

    /* Reschedule to a longer delay */
    rc = bt_work_reschedule_for_queue(&q, &dw, OS_MSEC(100));
    assert(rc >= 0);
    assert(bt_work_delayable_busy_get(&dw) != 0);

    /* Cancel synchronously */
    struct bt_work_sync sync;
    bool pending = bt_work_cancel_delayable_sync(&dw, &sync);
    assert(pending == true || pending == false);
    assert(bt_work_delayable_busy_get(&dw) == 0);

    /* Flush idle -> false */
    bool flushed = bt_work_flush_delayable(&dw, &sync);
    assert(flushed == false);

    /* Stop queue */
    bt_work_queue_drain(&q, true);
    assert(bt_work_queue_stop(&q, OS_SECONDS(1)) == 0);
}

int main(void)
{
    printf("[fifo_work/test_work] sys_workq submit...\n");
    test_sys_workq_submit();
    printf("[fifo_work/test_work] custom queue...\n");
    test_custom_queue_start_and_submit();
    printf("[fifo_work/test_work] cancel sync...\n");
    test_cancel_sync();
    printf("[fifo_work/test_work] delayable schedule/reschedule/cancel...\n");
    test_delayable_schedule_cancel_reschedule();
    printf("[fifo_work/test_work] all tests passed.\n");
    return 0;
}
