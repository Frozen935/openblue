/*
 * POSIX OS abstraction layer implementation
 */
#include "os.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <osdep/os.h>

/* Internal helper: convert ms to absolute timespec for timed waits (CLOCK_REALTIME) */
static int ms_to_abs_timespec(int32_t timeout_ms, struct timespec *ts_out)
{
	if (timeout_ms < 0) {
		return -1; /* forever */
	}
	struct timespec now;
	if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
		return -errno;
	}
	int64_t nsec_add = (int64_t)timeout_ms * 1000000LL;
	ts_out->tv_sec = now.tv_sec + (time_t)(nsec_add / 1000000000LL);
	ts_out->tv_nsec = now.tv_nsec + (long)(nsec_add % 1000000000LL);
	if (ts_out->tv_nsec >= 1000000000L) {
		ts_out->tv_sec += 1;
		ts_out->tv_nsec -= 1000000000L;
	}
	return 0;
}

/* Semaphore */
int os_sem_init(os_sem_t *sem, unsigned int initial_count, unsigned int limit)
{
	if (!sem) {
		return -EINVAL;
	}
	if (sem_init(&sem->sem, 0, initial_count) != 0) {
		return -errno;
	}
	sem->limit = limit == 0 ? OS_SEM_MAX_LIMIT : limit;
	return 0;
}

int os_sem_take(os_sem_t *sem, int32_t timeout_ms)
{
	if (!sem) {
		return -EINVAL;
	}
	if (timeout_ms == OS_TIMEOUT_NO_WAIT) {
		if (sem_trywait(&sem->sem) == 0) {
			return 0;
		}
		return (errno == EAGAIN) ? -ETIMEDOUT : -errno;
	}
	if (timeout_ms == OS_TIMEOUT_FOREVER) {
		while (1) {
			if (sem_wait(&sem->sem) == 0) {
				return 0;
			}
			if (errno == EINTR) {
				continue;
			}
			return -errno;
		}
	}
	struct timespec abstime;
	int rc = ms_to_abs_timespec(timeout_ms, &abstime);
	if (rc < 0) {
		/* treat as forever if conversion failed due to negative */
		while (1) {
			if (sem_wait(&sem->sem) == 0) {
				return 0;
			}
			if (errno == EINTR) {
				continue;
			}
			return -errno;
		}
	} else if (rc != 0) {
		return rc;
	}
	if (sem_timedwait(&sem->sem, &abstime) == 0) {
		return 0;
	}
	if (errno == ETIMEDOUT) {
		return -ETIMEDOUT;
	}
	return -errno;
}

int os_sem_give(os_sem_t *sem)
{
	if (!sem) {
		return -EINVAL;
	}
	/* Saturate at limit if provided */
	int val = 0;
	if (sem_getvalue(&sem->sem, &val) == 0) {
		/* Only check for saturation if the value is non-negative */
		if (val >= 0 && val >= sem->limit) {
			return 0; /* saturate */
		}
	}
	if (sem_post(&sem->sem) == 0) {
		return 0;
	}
	return -errno;
}

unsigned int os_sem_count_get(os_sem_t *sem)
{
	if (!sem) {
		return 0;
	}
	int val = 0;
	if (sem_getvalue(&sem->sem, &val) != 0) {
		return 0;
	}
	return (val < 0) ? 0U : (unsigned int)val;
}

int os_sem_reset(os_sem_t *sem)
{
	if (!sem) {
		return -EINVAL;
	}
	/* Drain semaphore to zero */
	while (1) {
		if (sem_trywait(&sem->sem) != 0) {
			if (errno == EAGAIN) {
				return 0; /* reached zero */
			}
			return -errno;
		}
	}
}

/* Mutex */
int os_mutex_init(os_mutex_t *mutex)
{
	if (!mutex) {
		return -EINVAL;
	}
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) != 0) {
		return -errno;
	}
	/* Default attributes */
	int rc = pthread_mutex_init(mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	return rc == 0 ? 0 : -errno;
}

int os_mutex_lock(os_mutex_t *mutex, int32_t timeout_ms)
{
	if (!mutex) {
		return -EINVAL;
	}
	if (timeout_ms == OS_TIMEOUT_NO_WAIT) {
		int rc = pthread_mutex_trylock(mutex);
		if (rc == 0) {
			return 0;
		}
		return (rc == EBUSY) ? -ETIMEDOUT : -rc;
	}
	if (timeout_ms == OS_TIMEOUT_FOREVER) {
		int rc;
		while ((rc = pthread_mutex_lock(mutex)) == EINTR) {
		}
		return rc == 0 ? 0 : -rc;
	}
#if defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS >= 200112L)
	struct timespec abstime;
	int conv = ms_to_abs_timespec(timeout_ms, &abstime);
	if (conv != 0) {
		return conv;
	}
	int rc = pthread_mutex_timedlock(mutex, &abstime);
	if (rc == 0) {
		return 0;
	}
	return (rc == ETIMEDOUT) ? -ETIMEDOUT : -rc;
#else
	/* Fallback: blocking lock for lack of timedlock */
	(void)timeout_ms;
	int rc;
	while ((rc = pthread_mutex_lock(mutex)) == EINTR) {
	}
	return rc == 0 ? 0 : -rc;
#endif
}

int os_mutex_unlock(os_mutex_t *mutex)
{
	if (!mutex) {
		return -EINVAL;
	}
	int rc = pthread_mutex_unlock(mutex);
	return rc == 0 ? 0 : -rc;
}

/* Condition variable */
int os_cond_init(os_cond_t *cond)
{
	if (!cond) {
		return -EINVAL;
	}
	pthread_condattr_t attr;
	if (pthread_condattr_init(&attr) != 0) {
		return -errno;
	}
	/* Default clock REALTIME */
	int rc = pthread_cond_init(cond, &attr);
	pthread_condattr_destroy(&attr);
	return rc == 0 ? 0 : -errno;
}

int os_cond_wait(os_cond_t *cond, os_mutex_t *mutex, int32_t timeout_ms)
{
	if (!cond || !mutex) {
		return -EINVAL;
	}
	if (timeout_ms == OS_TIMEOUT_NO_WAIT) {
		/* Non-blocking wait is meaningless; return timeout */
		return -ETIMEDOUT;
	}
	if (timeout_ms == OS_TIMEOUT_FOREVER) {
		int rc;
		while ((rc = pthread_cond_wait(cond, mutex)) == EINTR) {
		}
		return rc == 0 ? 0 : -rc;
	}
	struct timespec abstime;
	int conv = ms_to_abs_timespec(timeout_ms, &abstime);
	if (conv != 0) {
		return conv;
	}
	int rc = pthread_cond_timedwait(cond, mutex, &abstime);
	if (rc == 0) {
		return 0;
	}
	return (rc == ETIMEDOUT) ? -ETIMEDOUT : -rc;
}

int os_cond_signal(os_cond_t *cond)
{
	if (!cond) {
		return -EINVAL;
	}
	int rc = pthread_cond_signal(cond);
	return rc == 0 ? 0 : -rc;
}

int os_cond_broadcast(os_cond_t *cond)
{
	if (!cond) {
		return -EINVAL;
	}
	int rc = pthread_cond_broadcast(cond);
	return rc == 0 ? 0 : -rc;
}

/* Thread */
struct os_thread_start_pair {
	void (*fn)(void *);
	void *arg;
};
static void *start_trampoline(void *arg_pair)
{
	struct os_thread_start_pair *pp = (struct os_thread_start_pair *)arg_pair;
	void (*fn)(void *) = pp->fn;
	void *arg = pp->arg;
	/* Free the trampoline argument storage before invoking user fn to avoid leak */
	free(pp);
	if (fn) {
		fn(arg);
	}

	return NULL;
}

int os_thread_create(os_thread_t *thr, void (*start_routine)(void *), void *arg, const char *name,
		     int priority, size_t stack_size)
{
	(void)priority;   /* POSIX priority not managed here */
	(void)stack_size; /* Stack size can be set via attributes if needed */
	/* Delay is not supported natively; best-effort: start immediately */
	if (!thr || !start_routine) {
		return -EINVAL;
	}

	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0) {
		return -errno;
	}

	if (stack_size > 0) {
		(void)pthread_attr_setstacksize(&attr, stack_size);
	}

	struct os_thread_start_pair *pair = (struct os_thread_start_pair *)malloc(sizeof(*pair));
	if (!pair) {
		pthread_attr_destroy(&attr);
		return -ENOMEM;
	}
	pair->fn = start_routine;
	pair->arg = arg;
	int rc = pthread_create(&thr->thread, &attr, start_trampoline, (void *)pair);
	if (rc != 0) {
		pthread_attr_destroy(&attr);
		return -rc;
	}

	if (name && pthread_setname_np(thr->thread, name) != 0) {
		pthread_cancel(thr->thread);
		pthread_attr_destroy(&attr);
		return -errno;
	}

	pthread_attr_destroy(&attr);
	return rc == 0 ? 0 : -rc;
}

int os_thread_start(os_thread_t *thr)
{
	/* POSIX threads start at creation; provide no-op */
	(void)thr;
	return 0;
}

int os_thread_cancel(os_thread_t *thr)
{
	if (!thr || thr->thread == 0) {
		return -EINVAL;
	}
	int rc = pthread_cancel(thr->thread);
	return rc == 0 ? 0 : -rc;
}

int os_thread_join(os_thread_t *thr, int32_t timeout_ms)
{
	if (!thr) {
		return -EINVAL;
	}
	if (timeout_ms == OS_TIMEOUT_NO_WAIT) {
		return -ETIMEDOUT; /* non-blocking join not supported */
	}
	/* POSIX pthread_join does not support timeout. For interface completeness,
	 * implement blocking join irrespective of timeout.
	 */
	void *ret = NULL;
	int rc = pthread_join(thr->thread, &ret);
	return rc == 0 ? 0 : -rc;
}

os_tid_t os_thread_self(void)
{
	return pthread_self();
}

bool os_thread_is_current(os_thread_t *thr)
{
	return thr->thread == pthread_self();
}

int os_thread_yield(void)
{
	return sched_yield();
}

int os_thread_name_set(os_thread_t *thr, const char *name)
{
#if defined(__APPLE__) || defined(__MACH__)
	(void)thr;
	(void)name;
	return 0; /* pthread_setname_np variants differ; skip */
#elif defined(__linux__)
	if (!thr || !name) {
		return -EINVAL;
	}
	int rc = pthread_setname_np(thr->thread, name);
	return rc == 0 ? 0 : -rc;
#else
	(void)thr;
	(void)name;
	return 0; /* Best-effort no-op */
#endif
}

static pthread_mutex_t os_critical = PTHREAD_MUTEX_INITIALIZER;

void os_enter_critical(void)
{
	pthread_mutex_lock(&os_critical);
}

void os_exit_critical(void)
{
	pthread_mutex_unlock(&os_critical);
}

void os_sched_lock(void)
{
	os_enter_critical();
}

void os_sched_unlock(void)
{
	os_exit_critical();
}

void os_sleep_ms(uint32_t ms)
{
	usleep(ms * 1000);
}

uint64_t os_time_get_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static void timer_notify_callback(union sigval value)
{
	os_timer_t *timer = (os_timer_t *)value.sival_ptr;

	if (timer->cb) {
		timer->cb(timer, timer->arg);
	}
}

static int os_timer_activate(os_timer_t *timer, uint32_t timeout_ms)
{
	struct itimerspec itp = {};

	itp.it_value.tv_sec = timeout_ms / 1000;
	itp.it_value.tv_nsec = (timeout_ms % 1000) * 1000 * 1000;

	return timer_settime(timer->posix_timer, 0, &itp, NULL);
}

int os_timer_create(os_timer_t *timer, os_timer_cb_t cb, void *arg)
{
	struct sigevent sev;

	if (!timer || !cb) {
		return -EINVAL;
	}

	timer->cb = cb;
	timer->arg = arg;

	memset(&sev, 0, sizeof(sev));
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_signo = SIGALRM;
	sev.sigev_notify_function = timer_notify_callback;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = timer;
	if (timer_create(CLOCK_MONOTONIC, &sev, &timer->posix_timer) != 0) {
		return -errno;
	}

	/* Success */
	return 0;
}

int os_timer_start(os_timer_t *timer, uint32_t timeout_ms)
{
	if (!timer) {
		return -EINVAL;
	}

	return os_timer_activate(timer, timeout_ms);
}

int os_timer_stop(os_timer_t *timer)
{
	if (!timer) {
		return -EINVAL;
	}

	return os_timer_activate(timer, 0);
}

int os_timer_delete(os_timer_t *timer)
{
	if (!timer) {
		return -EINVAL;
	}

	return timer_delete(timer->posix_timer);
}

uint64_t os_timer_remaining_ms(const os_timer_t *timer)
{
	if (!timer) {
		return 0;
	}

	struct itimerspec itp = {};
	timer_gettime((timer_t)timer->posix_timer, &itp);

	return (uint64_t)(itp.it_value.tv_sec * 1000 + itp.it_value.tv_nsec / 1000000);
}

void *os_malloc(size_t size)
{
	return malloc(size);
}

void *os_calloc(size_t num, size_t size)
{
	return calloc(num, size);
}

void os_free(void *ptr)
{
	free(ptr);
}
