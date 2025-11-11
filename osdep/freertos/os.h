/*
 * FreeRTOS OS abstraction layer for threads, semaphores, mutexes, and condition variables.
 *
 * This header defines the "os_" prefixed interfaces identical to the POSIX variant,
 * implemented on top of FreeRTOS APIs.
 */
#ifndef OSDEP_FREERTOS_OS_H
#define OSDEP_FREERTOS_OS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

/* FreeRTOS kernel includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Timeout helpers (milliseconds) */
/* Timeout helpers (milliseconds) */
typedef int32_t os_timeout_t;

#define OS_TIMEOUT_NO_WAIT ((os_timeout_t)0)
#define OS_TIMEOUT_FOREVER ((os_timeout_t) - 1)

#define OS_USEC(us)     ((os_timeout_t)(((us) + 999) / 1000))
#define OS_MSEC(ms)     ((os_timeout_t)(ms))
#define OS_SECONDS(sec) ((os_timeout_t)((sec) * 1000))
#define OS_HOURS(h)     ((os_timeout_t)((h) * 60 * 60 * 1000))

#define OS_SEM_MAX_LIMIT (UINT32_MAX)

/* Semaphore: wrapper structure to track limit and FreeRTOS handle */
typedef struct os_sem {
	SemaphoreHandle_t handle; /* Counting semaphore handle */
	UBaseType_t limit;        /* Maximum count (saturation). */
} os_sem_t;

/* Mutex: use FreeRTOS mutex (binary semaphore) handle */
typedef SemaphoreHandle_t os_mutex_t;

/* Condition variable emulation via Event Groups */
typedef struct os_cond {
	EventGroupHandle_t group;
	EventBits_t bit; /* Bit used to signal/broadcast */
} os_cond_t;

/* Static initializers
 * Note: FreeRTOS objects are created at runtime; static initializers are placeholders.
 */
#define OS_MUTEX_INITIALIZER (NULL)
#define OS_COND_INITIALIZER  ((os_cond_t){.group = NULL, .bit = 0})

/* Convenience to define static mutex/cond */
#define OS_MUTEX_DEFINE(name) static os_mutex_t name = OS_MUTEX_INITIALIZER
#define OS_COND_DEFINE(name)  static os_cond_t name = OS_COND_INITIALIZER

#define OS_SEM_DEFINE(name, initial, max_limit)                                                    \
	static os_sem_t name;                                                                      \
	static void __os_init_sem_##name(void) __attribute__((constructor));                       \
	static void __os_init_sem_##name(void)                                                     \
	{                                                                                          \
		os_sem_init(&name, (unsigned int)(initial), (unsigned int)(max_limit));            \
	}

#define OS_PRIORITY(prio) (prio > configMAX_PRIORITIES ? (configMAX_PRIORITIES - 1) : (prio))
/* Thread abstraction */
typedef struct os_thread {
	TaskHandle_t task;
	EventGroupHandle_t join_group; /* Used to emulate join */
	EventBits_t join_bit;          /* Bit set when task exits */
	StaticTask_t task_buffer;      /* Buffer for static task allocation */
} os_thread_t;

/* Thread ID type */
typedef TaskHandle_t os_tid_t;

typedef struct os_timer os_timer_t;
typedef void (*os_timer_cb_t)(os_timer_t *timer, void *arg);
/* Timer structure */
typedef struct os_timer {
	TimerHandle_t handle;
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
unsigned int os_sem_count_get(os_sem_t *sem); /* via uxSemaphoreGetCount */
int os_sem_reset(os_sem_t *sem);              /* drain to zero */

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
int os_thread_start(os_thread_t *thr);  /* No-op: FreeRTOS tasks run when created */
int os_thread_cancel(os_thread_t *thr); /* 0 success, -EINVAL or -EPERM */
int os_thread_join(os_thread_t *thr, int32_t timeout_ms); /* 0 success, -ETIMEDOUT or -errno */
os_tid_t os_thread_self(void);
bool os_thread_is_current(os_thread_t *thr);
int os_thread_yield(void);
int os_thread_name_set(os_thread_t *thr, const char *name); /* Best-effort no-op */

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

#endif /* OSDEP_FREERTOS_OS_H */
