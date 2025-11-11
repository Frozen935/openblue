#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "base/queue/bt_lifo.h"

static void test_lifo_order(void)
{
    struct bt_lifo l; bt_lifo_init(&l);
    int a=1,b=2,c=3;
    bt_lifo_put(&l, &a);
    bt_lifo_put(&l, &b);
    bt_lifo_put(&l, &c);
    assert(bt_lifo_get(&l, OS_TIMEOUT_NO_WAIT) == &c);
    assert(bt_lifo_get(&l, OS_TIMEOUT_NO_WAIT) == &b);
    assert(bt_lifo_get(&l, OS_TIMEOUT_NO_WAIT) == &a);
    assert(bt_lifo_get(&l, OS_TIMEOUT_NO_WAIT) == NULL);
}

int main(void)
{
    printf("[fifo_work/test_lifo] order...\n");
    test_lifo_order();
    printf("[fifo_work/test_lifo] all tests passed.\n");
    return 0;
}
