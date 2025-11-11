#ifndef __OS_ATOMIC_H__
#define __OS_ATOMIC_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== Feature detection ===== */
#if 0
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
#if defined(__has_include)
#if __has_include(<stdatomic.h>)
#define OS_ATOMIC_HAVE_C11 1
#endif
#else
#define OS_ATOMIC_HAVE_C11 1
#endif
#endif

#if !defined(OS_ATOMIC_HAVE_C11)
#define OS_ATOMIC_HAVE_C11 0
#endif
#endif

#if !OS_ATOMIC_HAVE_C11
/* Prefer GCC/Clang builtins when C11 is not available */
#if defined(__clang__) || defined(__GNUC__)
#define OS_ATOMIC_HAVE_GNU_BUILTINS 1
#else
#define OS_ATOMIC_HAVE_GNU_BUILTINS 0
#endif
#endif

#if !OS_ATOMIC_HAVE_C11 && !OS_ATOMIC_HAVE_GNU_BUILTINS
#define OS_ATOMIC_HAVE_PTHREAD 1
#else
#define OS_ATOMIC_HAVE_PTHREAD 0
#endif

#if OS_ATOMIC_HAVE_C11
#include <stdatomic.h>
#endif
#if OS_ATOMIC_HAVE_PTHREAD
#include <pthread.h>
// static pthread_mutex_t os_atomic_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* ===== Memory order abstraction ===== */
enum os_atomic_order {
	OS_ATOMIC_RELAXED,
	OS_ATOMIC_ACQUIRE,
	OS_ATOMIC_RELEASE,
	OS_ATOMIC_ACQ_REL,
	OS_ATOMIC_SEQ_CST,
};

/* Map order to backend constants */
static inline int os_atomic__gnu_order(enum os_atomic_order order)
{
#if defined(__clang__) || defined(__GNUC__)
	switch (order) {
	case OS_ATOMIC_RELAXED:
		return __ATOMIC_RELAXED;
	case OS_ATOMIC_ACQUIRE:
		return __ATOMIC_ACQUIRE;
	case OS_ATOMIC_RELEASE:
		return __ATOMIC_RELEASE;
	case OS_ATOMIC_ACQ_REL:
		return __ATOMIC_ACQ_REL;
	case OS_ATOMIC_SEQ_CST:
	default:
		return __ATOMIC_SEQ_CST;
	}
#else
	(void)order;
	return 0;
#endif
}

#if OS_ATOMIC_HAVE_C11
static inline memory_order os_atomic__c11_order(enum os_atomic_order order)
{
	switch (order) {
	case OS_ATOMIC_RELAXED:
		return memory_order_relaxed;
	case OS_ATOMIC_ACQUIRE:
		return memory_order_acquire;
	case OS_ATOMIC_RELEASE:
		return memory_order_release;
	case OS_ATOMIC_ACQ_REL:
		return memory_order_acq_rel;
	case OS_ATOMIC_SEQ_CST:
	default:
		return memory_order_seq_cst;
	}
}
#endif

/* These operate on "plain" objects. Use builtins/pthread. */
static inline unsigned long os_atomic_word_load(const unsigned long *obj,
						enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_load_n(obj, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	/* Best-effort: no ordering without locking; assume single threaded in os */
	(void)order;
	return *obj;
#else
	(void)order;
	return *obj;
#endif
}

static inline void os_atomic_word_store(unsigned long *obj, unsigned long val,
					enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	__atomic_store_n(obj, val, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	*obj = val;
#else
	(void)order;
	*obj = val;
#endif
}

static inline unsigned long os_atomic_word_exchange(unsigned long *obj, unsigned long val,
						    enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_exchange_n(obj, val, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	unsigned long old = *obj;
	*obj = val;
	return old;
#else
	(void)order;
	unsigned long old = *obj;
	*obj = val;
	return old;
#endif
}

static inline bool os_atomic_word_compare_exchange(unsigned long *obj, unsigned long expected,
						   unsigned long desired,
						   enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	unsigned long exp = expected;
	return __atomic_compare_exchange_n(obj, &exp, desired, false, os_atomic__gnu_order(order),
					   os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	if (*obj == expected) {
		*obj = desired;
		return true;
	}
	return false;
#else
	(void)order;
	if (*obj == expected) {
		*obj = desired;
		return true;
	}
	return false;
#endif
}

static inline unsigned long os_atomic_word_fetch_add(unsigned long *obj, unsigned long arg,
						     enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_fetch_add(obj, arg, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	unsigned long old = *obj;
	*obj = old + arg;
	return old;
#else
	(void)order;
	unsigned long old = *obj;
	*obj = old + arg;
	return old;
#endif
}

static inline unsigned long os_atomic_word_fetch_sub(unsigned long *obj, unsigned long arg,
						     enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_fetch_sub(obj, arg, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	unsigned long old = *obj;
	*obj = old - arg;
	return old;
#else
	(void)order;
	unsigned long old = *obj;
	*obj = old - arg;
	return old;
#endif
}

static inline unsigned long os_atomic_word_fetch_or(unsigned long *obj, unsigned long arg,
						    enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_fetch_or(obj, arg, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	unsigned long old = *obj;
	*obj = old | arg;
	return old;
#else
	(void)order;
	unsigned long old = *obj;
	*obj = old | arg;
	return old;
#endif
}

static inline unsigned long os_atomic_word_fetch_and(unsigned long *obj, unsigned long arg,
						     enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_fetch_and(obj, arg, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	unsigned long old = *obj;
	*obj = old & arg;
	return old;
#else
	(void)order;
	unsigned long old = *obj;
	*obj = old & arg;
	return old;
#endif
}

static inline unsigned long os_atomic_word_fetch_xor(unsigned long *obj, unsigned long arg,
						     enum os_atomic_order order)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_fetch_xor(obj, arg, os_atomic__gnu_order(order));
#elif OS_ATOMIC_HAVE_PTHREAD
	(void)order;
	unsigned long old = *obj;
	*obj = old ^ arg;
	return old;
#else
	(void)order;
	unsigned long old = *obj;
	*obj = old ^ arg;
	return old;
#endif
}

static inline void *os_atomic_ptr_get(void *volatile *ptr)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
#else
	return *(void **)ptr;
#endif
}

static inline void *os_atomic_ptr_set(void *volatile *ptr, void *val)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
#else
	void *old = *(void **)ptr;
	*(void **)ptr = val;
	return old;
#endif
}

static inline void *os_atomic_ptr_clear(void *volatile *ptr)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	return __atomic_exchange_n(ptr, NULL, __ATOMIC_SEQ_CST);
#else
	void *old = *(void **)ptr;
	*(void **)ptr = NULL;
	return old;
#endif
}

static inline bool os_atomic_ptr_cas(void *volatile *ptr, void *expected, void *desired)
{
#if OS_ATOMIC_HAVE_GNU_BUILTINS
	void *exp = expected;
	return __atomic_compare_exchange_n(ptr, &exp, desired, false, __ATOMIC_SEQ_CST,
					   __ATOMIC_SEQ_CST);
#else
	if (*(void **)ptr == expected) {
		*(void **)ptr = desired;
		return true;
	}
	return false;
#endif
}

#endif /* _OS_ATOMIC_H */
