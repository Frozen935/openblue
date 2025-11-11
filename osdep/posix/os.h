#ifndef OSDEP_POSIX_OS_H
#define OSDEP_POSIX_OS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Timeout helpers (milliseconds) */
typedef int32_t os_timeout_t;

#define OS_TIMEOUT_NO_WAIT ((os_timeout_t)0)
#define OS_TIMEOUT_FOREVER ((os_timeout_t) - 1)

#define OS_USEC(us)     ((os_timeout_t)(((us) + 999) / 1000))
#define OS_MSEC(ms)     ((os_timeout_t)(ms))
#define OS_SECONDS(sec) ((os_timeout_t)((sec) * 1000))
#define OS_HOURS(h)     ((os_timeout_t)((h) * 60 * 60 * 1000))

/* Semaphore: wrapper structure to track optional limit and use POSIX sem */
typedef struct os_sem {
	/* POSIX unnamed semaphore */
	sem_t sem;
	/* Maximum count (saturation). 0 means no explicit cap beyond UINT_MAX. */
	unsigned int limit;
} os_sem_t;

#define OS_SEM_MAX_LIMIT (UINT32_MAX)

/* Mutex and condition variable: directly typedef to POSIX types for simple initialization */
typedef pthread_mutex_t os_mutex_t;
typedef pthread_cond_t os_cond_t;

/* Static initializers (for use in global/struct initializers) */
#define OS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define OS_COND_INITIALIZER  PTHREAD_COND_INITIALIZER

/* Convenience to define static mutex/cond */
#define OS_MUTEX_DEFINE(name) static os_mutex_t name = OS_MUTEX_INITIALIZER
#define OS_COND_DEFINE(name)  static os_cond_t name = OS_COND_INITIALIZER

/* Note: relies on GCC/Clang constructor extension. */
#define OS_SEM_DEFINE(name, initial, max_limit) os_sem_t name

#define OS_PRIORITY(prio) (100 + (prio))
/* Thread abstraction */
typedef struct os_thread {
	pthread_t thread;
} os_thread_t;

/* Thread ID type */
typedef pthread_t os_tid_t;

typedef struct os_timer os_timer_t;
typedef void (*os_timer_cb_t)(os_timer_t *timer, void *arg);

typedef struct os_timer {
	timer_t posix_timer;
	os_timer_cb_t cb;
	void *arg;
} os_timer_t;

/* Timer functions */
int os_timer_create(os_timer_t *timer, os_timer_cb_t cb, void *arg);
int os_timer_start(os_timer_t *timer, uint32_t timeout_ms);
int os_timer_stop(os_timer_t *timer);
int os_timer_delete(os_timer_t *timer);
uint64_t os_timer_remaining_ms(const os_timer_t *timer);

/* Semaphore functions */
int os_sem_init(os_sem_t *sem, unsigned int initial_count, unsigned int limit);
int os_sem_take(os_sem_t *sem,
		int32_t timeout_ms);          /* 0 success, -ETIMEDOUT timeout, other -errno */
int os_sem_give(os_sem_t *sem);               /* 0 success, -errno on failure */
unsigned int os_sem_count_get(os_sem_t *sem); /* Best-effort via sem_getvalue */
int os_sem_reset(os_sem_t *sem);              /* Set count to 0 by draining */

/* Mutex functions */
int os_mutex_init(os_mutex_t *mutex);
int os_mutex_lock(os_mutex_t *mutex, int32_t timeout_ms); /* 0 success, -ETIMEDOUT, other -errno */
int os_mutex_unlock(os_mutex_t *mutex);                   /* 0 success, -errno */

/* Condition variable functions */
int os_cond_init(os_cond_t *cond);
int os_cond_wait(os_cond_t *cond, os_mutex_t *mutex,
		 int32_t timeout_ms); /* 0 success, -ETIMEDOUT or -errno */
int os_cond_signal(os_cond_t *cond);
int os_cond_broadcast(os_cond_t *cond);

/* Thread functions */
int os_thread_create(os_thread_t *thr, void (*start_routine)(void *), void *arg, const char *name,
		     int priority, size_t stack_size);
int os_thread_start(os_thread_t *thr);  /* No-op for POSIX (thread starts in create) */
int os_thread_cancel(os_thread_t *thr); /* 0 success, -EINVAL or -EPERM */
int os_thread_join(os_thread_t *thr, int32_t timeout_ms); /* 0 success, -ETIMEDOUT or -errno */
os_tid_t os_thread_self(void);
bool os_thread_is_current(os_thread_t *thr);
int os_thread_yield(void);
int os_thread_name_set(os_thread_t *thr, const char *name);

/* Critical section functions */
void os_enter_critical(void);
void os_exit_critical(void);

/* Scheduler lock/unlock functions */
void os_sched_lock(void);
void os_sched_unlock(void);

/* Time functions */
void os_sleep_ms(uint32_t ms);
uint64_t os_time_get_ms(void);

/* Memory allocation functions */
void *os_malloc(size_t size);
void *os_calloc(size_t num, size_t size);
void os_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* OSDEP_POSIX_OS_H */
