/*
 * FreeRTOS OS abstraction layer implementation.
 */

#include <string.h>

#include "osdep/os.h"

/* FreeRTOS kernel includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

/* Internal helper: ms to ticks */
static TickType_t ms_to_ticks(int32_t timeout_ms)
{
	if (timeout_ms < 0) {
		return portMAX_DELAY;
	}
	return (timeout_ms == 0) ? 0 : pdMS_TO_TICKS((uint32_t)timeout_ms);
}

/* Semaphore */
int os_sem_init(os_sem_t *sem, unsigned int initial_count, unsigned int limit)
{
	if (!sem) {
		return -EINVAL;
	}
	UBaseType_t max_lim = (limit == 0) ? (UBaseType_t)OS_SEM_MAX_LIMIT : (UBaseType_t)limit;
	SemaphoreHandle_t h = xSemaphoreCreateCounting(max_lim, (UBaseType_t)initial_count);
	if (h == NULL) {
		return -ENOMEM;
	}
	sem->handle = h;
	sem->limit = max_lim;
	return 0;
}

int os_sem_take(os_sem_t *sem, int32_t timeout_ms)
{
	if (!sem || !sem->handle) {
		return -EINVAL;
	}
	TickType_t ticks = ms_to_ticks(timeout_ms);
	BaseType_t ok = xSemaphoreTake(sem->handle, ticks);
	if (ok == pdTRUE) {
		return 0;
	}
	return (timeout_ms == OS_TIMEOUT_NO_WAIT) ? -ETIMEDOUT : -ETIMEDOUT;
}

int os_sem_give(os_sem_t *sem)
{
	if (!sem || !sem->handle) {
		return -EINVAL;
	}
	/* Saturate at limit (uxSemaphoreGetCount available since FreeRTOS 8+) */
	UBaseType_t cnt = uxSemaphoreGetCount(sem->handle);
	if (cnt >= sem->limit) {
		return 0; /* saturation: no change */
	}
	BaseType_t ok = xSemaphoreGive(sem->handle);
	return (ok == pdTRUE) ? 0 : -EAGAIN;
}

unsigned int os_sem_count_get(os_sem_t *sem)
{
	if (!sem || !sem->handle) {
		return 0U;
	}
	UBaseType_t cnt = uxSemaphoreGetCount(sem->handle);
	return (unsigned int)cnt;
}

int os_sem_reset(os_sem_t *sem)
{
	if (!sem || !sem->handle) {
		return -EINVAL;
	}
	/* Drain semaphore to zero */
	while (uxSemaphoreGetCount(sem->handle) > 0) {
		if (xSemaphoreTake(sem->handle, 0) != pdTRUE) {
			break; /* couldn't drain further */
		}
	}
	return 0;
}

/* Mutex */
int os_mutex_init(os_mutex_t *mutex)
{
	if (!mutex) {
		return -EINVAL;
	}
	SemaphoreHandle_t h = xSemaphoreCreateMutex();
	if (h == NULL) {
		return -ENOMEM;
	}
	*mutex = h;
	return 0;
}

int os_mutex_lock(os_mutex_t *mutex, int32_t timeout_ms)
{
	if (!mutex || *mutex == NULL) {
		return -EINVAL;
	}
	TickType_t ticks = ms_to_ticks(timeout_ms);
	BaseType_t ok = xSemaphoreTake(*mutex, ticks);
	if (ok == pdTRUE) {
		return 0;
	}
	return (timeout_ms == OS_TIMEOUT_NO_WAIT) ? -ETIMEDOUT : -ETIMEDOUT;
}

int os_mutex_unlock(os_mutex_t *mutex)
{
	if (!mutex || *mutex == NULL) {
		return -EINVAL;
	}
	BaseType_t ok = xSemaphoreGive(*mutex);
	return (ok == pdTRUE) ? 0 : -EPERM;
}

/* Condition variable emulation */
int os_cond_init(os_cond_t *cond)
{
	if (!cond) {
		return -EINVAL;
	}
	EventGroupHandle_t g = xEventGroupCreate();
	if (g == NULL) {
		return -ENOMEM;
	}
	cond->group = g;
	cond->bit = (EventBits_t)0x01; /* default bit */
	return 0;
}

int os_cond_wait(os_cond_t *cond, os_mutex_t *mutex, int32_t timeout_ms)
{
	if (!cond || cond->group == NULL) {
		return -EINVAL;
	}
	(void)mutex; /* No native association; caller must hold/unhold externally if needed */
	/* Clear the bit before waiting to ensure fresh wait */
	xEventGroupClearBits(cond->group, cond->bit);
	TickType_t ticks = ms_to_ticks(timeout_ms);
	EventBits_t bits = xEventGroupWaitBits(cond->group, cond->bit, pdTRUE /*clear on exit*/,
					       pdFALSE /*any*/, ticks);
	if ((bits & cond->bit) != 0) {
		return 0;
	}
	return -ETIMEDOUT;
}

int os_cond_signal(os_cond_t *cond)
{
	if (!cond || cond->group == NULL) {
		return -EINVAL;
	}
	EventBits_t bits = xEventGroupSetBits(cond->group, cond->bit);
	(void)bits;
	return 0;
}

int os_cond_broadcast(os_cond_t *cond)
{
	/* Event groups naturally wake all waiters when a bit is set */
	if (!cond || cond->group == NULL) {
		return -EINVAL;
	}
	EventBits_t bits = xEventGroupSetBits(cond->group, cond->bit);
	(void)bits;
	return 0;
}

/* Thread */
typedef struct os_thread_start_info {
	void (*fn)(void *);
	void *arg;
	EventGroupHandle_t join_group;
	EventBits_t join_bit;
	int32_t delay_ms;
} os_thread_start_info_t;

static void os_thread_trampoline(void *pv)
{
	os_thread_start_info_t *info = (os_thread_start_info_t *)pv;
	if (!info) {
		vTaskDelete(NULL);
		return;
	}
	if (info->delay_ms > 0) {
		vTaskDelay(pdMS_TO_TICKS((uint32_t)info->delay_ms));
	}

	if (info->fn) {
		info->fn(info->arg);
	}

	/* Signal join and cleanup */
	if (info->join_group != NULL) {
		xEventGroupSetBits(info->join_group, info->join_bit);
	}
	vPortFree(info);
	vTaskDelete(NULL);
}

int os_thread_create(os_thread_t *thr, void (*start_routine)(void *), void *arg, const char *name,
		     int priority, size_t stack_size)
{
	if (!thr || !start_routine) {
		return -EINVAL;
	}

	/* Prepare join group */
	EventGroupHandle_t g = xEventGroupCreate();
	if (g == NULL) {
		return -ENOMEM;
	}

	os_thread_start_info_t *info = (os_thread_start_info_t *)pvPortMalloc(sizeof(*info));
	if (!info) {
		vEventGroupDelete(g);
		return -ENOMEM;
	}
	info->fn = start_routine;
	info->arg = arg;
	info->join_group = g;
	info->join_bit = (EventBits_t)0x01;
	info->delay_ms = delay_ms;

	/* Convert stack size in bytes to FreeRTOS stack depth (words) */
	UBaseType_t stack_depth = (stack_size > 0) ? (UBaseType_t)(stack_size / sizeof(StackType_t))
						   : (UBaseType_t)configMINIMAL_STACK_SIZE;
	UBaseType_t prio = (priority < 0) ? (UBaseType_t)tskIDLE_PRIORITY : (UBaseType_t)priority;

	BaseType_t rc = xTaskCreate(os_thread_trampoline, name, stack_depth, (void *)info, prio,
				    &thr->task);
	if (rc != pdPASS) {
		vPortFree(info);
		vEventGroupDelete(g);
		thr->task = NULL;
		thr->join_group = NULL;
		thr->join_bit = 0;
		return -ENOMEM;
	}

	thr->join_group = g;
	thr->join_bit = info->join_bit;
	return 0;
}

int os_thread_start(os_thread_t *thr)
{
	/* FreeRTOS tasks start when created (once scheduler runs); provide no-op */
	(void)thr;
	return 0;
}

int os_thread_cancel(os_thread_t *thr)
{
	if (!thr || thr->task == NULL) {
		return -EINVAL;
	}
	BaseType_t rc = vTaskDelete(thr->task);
	if (rc != pdPASS) {
		return -EPERM;
	}
	thr->task = NULL;
	thr->join_group = NULL;
	thr->join_bit = 0;

	return 0;
}

int os_thread_join(os_thread_t *thr, int32_t timeout_ms)
{
	if (!thr || thr->join_group == NULL) {
		return -EINVAL;
	}
	TickType_t ticks = ms_to_ticks(timeout_ms);
	EventBits_t bits = xEventGroupWaitBits(thr->join_group, thr->join_bit, pdTRUE /*clear*/,
					       pdFALSE, ticks);
	if ((bits & thr->join_bit) != 0) {
		return 0;
	}
	return -ETIMEDOUT;
}

os_tid_t os_thread_self(void)
{
	return xTaskGetCurrentTaskHandle();
}

bool os_thread_is_current(os_thread_t *thr)
{
	return thr->task == xTaskGetCurrentTaskHandle();
}

int os_thread_yield(void)
{
	taskYIELD();

	return 0;
}

int os_thread_name_set(os_thread_t *thr, const char *name)
{
	/* FreeRTOS tasks have immutable names post-creation; provide best-effort no-op */
	(void)thr;
	(void)name;
	return 0;
}

void os_enter_critical(void)
{
	taskENTER_CRITICAL();
}

void os_exit_critical(void)
{
	taskEXIT_CRITICAL();
}

void os_sched_lock(void)
{
	(void)vTaskSuspendAll();
}

void os_sched_unlock(void)
{
	(void)xTaskResumeAll();
}

void os_sleep_ms(uint32_t ms)
{
	vTaskDelay(ms_to_ticks(ms));
}

uint64_t os_time_get_ms(void)
{
	TickType_t ticks = xTaskGetTickCount();

	return (uint64_t)ticks * portTICK_PERIOD_MS;
}

int os_timer_create(os_timer_t *timer, os_timer_cb_t cb, void *arg)
{
	if (!timer || !cb) {
		return -EINVAL;
	}

	timer->cb = cb;
	timer->arg = arg;

	timer->handle = xTimerCreate(NULL, pdMS_TO_TICKS(1), pdFALSE, timer, );
	if (timer->handle == NULL) {
		return -ENOMEM;
	}

	return 0;
}

int os_timer_start(os_timer_t *timer, uint32_t timeout_ms)
{
	if (!timer) {
		return -EINVAL;
	}

	xTimerChangePeriod(timer->handle, pdMS_TO_TICKS(timeout_ms));

	return xTimerStart(timer->handle, portMAX_DELAY);
}

int os_timer_stop(os_timer_t *timer)
{
	if (!timer) {
		return -EINVAL;
	}

	return xTimerStop(timer->handle, portMAX_DELAY);
}

int os_timer_delete(os_timer_t *timer)
{
	if (!timer) {
		return -EINVAL;
	}

	return xTimerDelete(timer->handle, portMAX_DELAY);
}

uint64_t os_timer_remaining_ms(const os_timer_t *timer)
{
	TickType_t remaining_ticks;
	if (!timer) {
		return -EINVAL;
	}

	remaining_ticks = xTimerGetExpiryTime(timer->handle) - xTaskGetTickCount();

	return (uint64_t)(remaining_ticks * portTICK_PERIOD_MS);
}

void *os_malloc(size_t size)
{
	return pvPortMalloc(size);
}

void *os_calloc(size_t num, size_t size)
{
	return pvPortCalloc(num, size);
}

void os_free(void *ptr)
{
	vPortFree(ptr);
}
