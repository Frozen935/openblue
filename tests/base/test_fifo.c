#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "base/queue/bt_fifo.h"
#include "osdep/os.h"

static void test_fifo_order_and_empty(void)
{
    struct bt_fifo f; bt_fifo_init(&f);
    int a=1,b=2,c=3;
    bt_fifo_put(&f, &a);
    bt_fifo_put(&f, &b);
    bt_fifo_put(&f, &c);
    assert(bt_fifo_is_empty(&f) == false);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &a);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &b);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &c);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == NULL);
    assert(bt_fifo_is_empty(&f) == true);
}

struct thr_arg { struct bt_fifo *f; void *out; };

static void consumer_block(void *arg)
{
    struct thr_arg *ta = (struct thr_arg *)arg;
    ta->out = bt_fifo_get(ta->f, OS_TIMEOUT_FOREVER);
}

static void test_fifo_blocking_get(void)
{
    struct bt_fifo f; bt_fifo_init(&f);
    struct thr_arg ta = { .f=&f, .out=NULL };
    os_thread_t thr;
    assert(os_thread_create(&thr, consumer_block, &ta, "cons", 0, 0) == 0);
    os_sleep_ms(50);
    static int v = 123;
    bt_fifo_put(&f, &v);
    assert(os_thread_join(&thr, OS_TIMEOUT_FOREVER) == 0);
    assert(ta.out == &v);
    assert(bt_fifo_is_empty(&f) == true);
}

static void test_fifo_timeout(void)
{
    struct bt_fifo f; bt_fifo_init(&f);
    void *v = bt_fifo_get(&f, OS_MSEC(100));
    assert(v == NULL);
}

static void test_fifo_peek(void)
{
    struct bt_fifo f; bt_fifo_init(&f);
    int x=10,y=20;
    bt_fifo_put(&f, &x);
    bt_fifo_put(&f, &y);
    assert(bt_fifo_peek_head(&f) == &x);
    assert(bt_fifo_peek_tail(&f) == &y);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &x);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &y);
}

int main(void)
{
    printf("[fifo_work/test_fifo] order + empty...\n");
    test_fifo_order_and_empty();
    printf("[fifo_work/test_fifo] blocking...\n");
    test_fifo_blocking_get();
    printf("[fifo_work/test_fifo] timeout...\n");
    test_fifo_timeout();
    printf("[fifo_work/test_fifo] peek...\n");
    test_fifo_peek();
    printf("[fifo_work/test_fifo] all tests passed.\n");
    return 0;
}
