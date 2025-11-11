#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "osdep/os.h"

static void test_sem_basic_and_limit(void)
{
    os_sem_t sem;
    assert(os_sem_init(&sem, 0, 2) == 0);

    /* Non-blocking take should timeout */
    assert(os_sem_take(&sem, OS_TIMEOUT_NO_WAIT) == -ETIMEDOUT);

    /* Give twice, saturation at limit */
    assert(os_sem_give(&sem) == 0);
    assert(os_sem_give(&sem) == 0);
    unsigned int c = os_sem_count_get(&sem);
    assert(c <= 2);

    /* Third give should saturate (no effect) */
    (void)os_sem_give(&sem);
    unsigned int c2 = os_sem_count_get(&sem);
    assert(c2 == c || c2 == 2);

    /* Take twice succeeds */
    assert(os_sem_take(&sem, OS_MSEC(10)) == 0);
    assert(os_sem_take(&sem, OS_MSEC(10)) == 0);
    /* Now timeout */
    assert(os_sem_take(&sem, OS_MSEC(10)) == -ETIMEDOUT);
    printf("sem basic and limit test passed.\n");
}

static void test_mutex_lock_unlock_and_timed(void)
{
    os_mutex_t m;
    assert(os_mutex_init(&m) == 0);

    /* Lock/unlock */
    assert(os_mutex_lock(&m, OS_TIMEOUT_FOREVER) == 0);
    assert(os_mutex_unlock(&m) == 0);

    /* Trylock should succeed when unlocked */
    assert(os_mutex_lock(&m, OS_TIMEOUT_NO_WAIT) == 0);

    /* Another timed trylock should timeout (already locked) */
    assert(os_mutex_lock(&m, OS_TIMEOUT_NO_WAIT) == -ETIMEDOUT);
    assert(os_mutex_unlock(&m) == 0);
    printf("mutex lock unlock timed test passed.\n");
}

/* --- Begin: Multi-threaded semaphore test --- */
#define NUM_SEM_THREADS 4
struct sem_test_ctx {
	os_sem_t sem;
	volatile int workers_done;
};

static void sem_worker(void *arg)
{
	struct sem_test_ctx *ctx = (struct sem_test_ctx *)arg;
	/* Wait until the main thread gives the semaphore */
	assert(os_sem_take(&ctx->sem, OS_TIMEOUT_FOREVER) == 0);
	__sync_fetch_and_add(&ctx->workers_done, 1);
}

static void test_sem_multithreaded(void)
{
	struct sem_test_ctx ctx = { 0 };
	os_thread_t threads[NUM_SEM_THREADS];

	/* Initialize semaphore with 0 count, so threads will block */
	assert(os_sem_init(&ctx.sem, 0, 0) == 0);

	for (int i = 0; i < NUM_SEM_THREADS; i++) {
		assert(os_thread_create(&threads[i], sem_worker, &ctx, "sem_w", 0, 0) == 0);
	}

	/* Give threads a moment to start and block on the semaphore */
	os_sleep_ms(50);
	assert(ctx.workers_done == 0);

	/* Unblock all threads */
	for (int i = 0; i < NUM_SEM_THREADS; i++) {
		assert(os_sem_give(&ctx.sem) == 0);
	}

	/* Join all threads */
	for (int i = 0; i < NUM_SEM_THREADS; i++) {
		assert(os_thread_join(&threads[i], OS_TIMEOUT_FOREVER) == 0);
	}

	/* Check that all workers completed their task */
	assert(ctx.workers_done == NUM_SEM_THREADS);
    printf("sem test passed, workers done: %d\n", ctx.workers_done);
}
/* --- End: Multi-threaded semaphore test --- */

/* --- Begin: Multi-threaded mutex test --- */
#define NUM_MUTEX_THREADS 4
#define INCREMENTS_PER_THREAD 100000

struct mutex_test_ctx {
	os_mutex_t mutex;
	volatile int counter;
};

static void mutex_worker(void *arg)
{
	struct mutex_test_ctx *ctx = (struct mutex_test_ctx *)arg;
	for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
		assert(os_mutex_lock(&ctx->mutex, OS_TIMEOUT_FOREVER) == 0);
		ctx->counter++;
		assert(os_mutex_unlock(&ctx->mutex) == 0);
	}
}

static void test_mutex_multithreaded(void)
{
	struct mutex_test_ctx ctx = { 0 };
	os_thread_t threads[NUM_MUTEX_THREADS];

	assert(os_mutex_init(&ctx.mutex) == 0);

	for (int i = 0; i < NUM_MUTEX_THREADS; i++) {
		assert(os_thread_create(&threads[i], mutex_worker, &ctx, "mtx_w", 0, 0) == 0);
	}

	/* Join all threads */
	for (int i = 0; i < NUM_MUTEX_THREADS; i++) {
		assert(os_thread_join(&threads[i], OS_TIMEOUT_FOREVER) == 0);
	}

	/* Verify the final counter value to check for race conditions */
	int expected = NUM_MUTEX_THREADS * INCREMENTS_PER_THREAD;
	assert(ctx.counter == expected);
    printf("mutex test passed, counter: %d\n", ctx.counter);
}
/* --- End: Multi-threaded mutex test --- */

struct cond_ctx { os_mutex_t m; os_cond_t c; bool ready; };

static void cond_signaler(void *arg)
{
    struct cond_ctx *ctx = (struct cond_ctx *)arg;
    os_sleep_ms(50);
    os_mutex_lock(&ctx->m, OS_TIMEOUT_FOREVER);
    ctx->ready = true;
    os_cond_signal(&ctx->c);
    os_mutex_unlock(&ctx->m);
}

static void test_cond_wait_signal_timeout(void)
{
    struct cond_ctx ctx;
    assert(os_mutex_init(&ctx.m) == 0);
    assert(os_cond_init(&ctx.c) == 0);
    ctx.ready = false;

    os_mutex_lock(&ctx.m, OS_TIMEOUT_FOREVER);
    /* Start signaler thread */
    os_thread_t thr;
    assert(os_thread_create(&thr, cond_signaler, &ctx, "sig", 0, 0) == 0);

    /* Wait with timeout longer than signaler delay */
    assert(os_cond_wait(&ctx.c, &ctx.m, OS_MSEC(200)) == 0);
    assert(ctx.ready == true);
    os_mutex_unlock(&ctx.m);
    assert(os_thread_join(&thr, OS_TIMEOUT_FOREVER) == 0);

    /* Test timeout when not signaled */
    os_mutex_lock(&ctx.m, OS_TIMEOUT_FOREVER);
    ctx.ready = false;
    /* No signal -> timeout */
    assert(os_cond_wait(&ctx.c, &ctx.m, OS_MSEC(50)) == -ETIMEDOUT);
    os_mutex_unlock(&ctx.m);
    printf("cond wait signal timeout test passed.\n");
}

static void worker(void *arg)
{
    int *p = (int *)arg;
    *p = 42;
}

static void test_thread_create_join_name(void)
{
    os_thread_t thr;
    int v = 0;
    assert(os_thread_create(&thr, worker, &v, "worker", 0, 0) == 0);
    int rcname = os_thread_name_set(&thr, "worker");
    assert(rcname == 0 || rcname == -EINVAL);
    assert(os_thread_join(&thr, OS_TIMEOUT_FOREVER) == 0);
    assert(v == 42);
    printf("thread create join name test passed.\n");
}

static volatile int timer_hits = 0;

static void timer_cb(os_timer_t *t, void *arg)
{
    (void)t; (void)arg;
    timer_hits++;
}

static void test_timer_start_stop_remaining(void)
{
    os_timer_t t;
    assert(os_timer_create(&t, timer_cb, NULL) == 0);
    assert(os_timer_start(&t, 100) == 0);
    /* Wait until callback fires */
    for (int i = 0; i < 30 && timer_hits == 0; i++) {
        os_sleep_ms(10);
    }
    assert(timer_hits >= 1);
    uint64_t rem = os_timer_remaining_ms(&t);
    assert(rem == 0 || rem < 100);
    assert(os_timer_stop(&t) == 0);
    assert(os_timer_delete(&t) == 0);
    printf("timer test passed, hits: %d\n", timer_hits);
}

static void test_time_sleep_monotonic(void)
{
    uint64_t t0 = os_time_get_ms();
    os_sleep_ms(50);
    uint64_t t1 = os_time_get_ms();
    assert(t1 > t0);
    printf("time sleep monotonic test passed, sleep: %llu ms\n", t1 - t0);
}

static void test_mem_alloc_free(void)
{
    void *p = os_malloc(128);
    assert(p != NULL);
    memset(p, 0xAB, 128);
    os_free(p);

    void *q = os_calloc(4, 32);
    assert(q != NULL);
    uint8_t *u = (uint8_t *)q;
    for (int i = 0; i < 128; i++) assert(u[i] == 0);
    os_free(q);
    printf("mem alloc free test passed.\n");
}

int main(void)
{
    printf("[posix/test_os] sem...\n");
    test_sem_basic_and_limit();
    test_sem_multithreaded();
    printf("[posix/test_os] mutex...\n");
    test_mutex_lock_unlock_and_timed();
    test_mutex_multithreaded();
    printf("[posix/test_os] cond...\n");
    test_cond_wait_signal_timeout();
    printf("[posix/test_os] thread...\n");
    test_thread_create_join_name();
    printf("[posix/test_os] timer...\n");
    test_timer_start_stop_remaining();
    printf("[posix/test_os] time...\n");
    test_time_sleep_monotonic();
    printf("[posix/test_os] mem...\n");
    test_mem_alloc_free();
    printf("[posix/test_os] all tests passed.\n");
    return 0;
}