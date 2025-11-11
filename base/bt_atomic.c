#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "osdep/atomic/atomic.h"

atomic_val_t atomic_get(const atomic_t *target)
{
	if (!target) {
		return 0;
	}
	return (atomic_val_t)os_atomic_word_load((const unsigned long *)target, OS_ATOMIC_SEQ_CST);
}

atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	if (!target) {
		return 0;
	}
	unsigned long old = os_atomic_word_exchange((unsigned long *)target, (unsigned long)value,
						    OS_ATOMIC_SEQ_CST);
	return (atomic_val_t)old;
}

atomic_val_t atomic_inc(atomic_t *target)
{
	if (!target) {
		return 0;
	}
	unsigned long old =
		os_atomic_word_fetch_add((unsigned long *)target, 1UL, OS_ATOMIC_SEQ_CST);
	return (atomic_val_t)old;
}

atomic_val_t atomic_dec(atomic_t *target)
{
	if (!target) {
		return 0;
	}
	unsigned long old =
		os_atomic_word_fetch_sub((unsigned long *)target, 1UL, OS_ATOMIC_SEQ_CST);
	return (atomic_val_t)old;
}

bool atomic_cas(atomic_t *target, atomic_val_t expected, atomic_val_t desired)
{
	if (!target) {
		return false;
	}
	return os_atomic_word_compare_exchange((unsigned long *)target, (unsigned long)expected,
					       (unsigned long)desired, OS_ATOMIC_SEQ_CST);
}

void atomic_add(atomic_t *target, atomic_val_t value)
{
	if (!target) {
		return;
	}
	(void)os_atomic_word_fetch_add((unsigned long *)target, (unsigned long)value,
				       OS_ATOMIC_SEQ_CST);
}

void atomic_sub(atomic_t *target, atomic_val_t value)
{
	if (!target) {
		return;
	}
	(void)os_atomic_word_fetch_sub((unsigned long *)target, (unsigned long)value,
				       OS_ATOMIC_SEQ_CST);
}

static inline size_t _atomic_word_index(int bit)
{
	return ((size_t)bit) / ATOMIC_BITS;
}

static inline atomic_t _atomic_bit_mask(int bit)
{
	return (atomic_t)(1UL << (bit % ATOMIC_BITS));
}

bool atomic_test_bit(const atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return false;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long word =
		os_atomic_word_load((const unsigned long *)&target[idx], OS_ATOMIC_SEQ_CST);
	return (word & (unsigned long)_atomic_bit_mask(bit)) != 0;
}

void atomic_set_bit(atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return;
	}
	size_t idx = _atomic_word_index(bit);
	(void)os_atomic_word_fetch_or((unsigned long *)&target[idx],
				      (unsigned long)_atomic_bit_mask(bit), OS_ATOMIC_SEQ_CST);
}

void atomic_set_bit_to(atomic_t *target, int bit, bool val)
{
	if (val) {
		atomic_set_bit(target, bit);
	} else {
		atomic_clear_bit(target, bit);
	}
}

void atomic_clear_bit(atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long mask = (unsigned long)_atomic_bit_mask(bit);
	(void)os_atomic_word_fetch_and((unsigned long *)&target[idx], ~mask, OS_ATOMIC_SEQ_CST);
}

bool atomic_test_and_set_bit(atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return false;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long mask = (unsigned long)_atomic_bit_mask(bit);
	unsigned long old =
		os_atomic_word_fetch_or((unsigned long *)&target[idx], mask, OS_ATOMIC_SEQ_CST);
	return (old & mask) != 0;
}

bool atomic_test_and_clear_bit(atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return false;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long mask = (unsigned long)_atomic_bit_mask(bit);
	unsigned long old =
		os_atomic_word_fetch_and((unsigned long *)&target[idx], ~mask, OS_ATOMIC_SEQ_CST);
	return (old & mask) != 0;
}

/* Atomic pointer helpers mapped to os/atomic implementation */
void *atomic_ptr_get(atomic_ptr_t *ptr)
{
	return os_atomic_ptr_get((void *volatile *)ptr);
}

void *atomic_ptr_set(atomic_ptr_t *ptr, void *val)
{
	return os_atomic_ptr_set((void *volatile *)ptr, val);
}

void *atomic_ptr_clear(atomic_ptr_t *ptr)
{
	return os_atomic_ptr_clear((void *volatile *)ptr);
}

bool atomic_ptr_cas(atomic_ptr_t *ptr, void *expected, void *desired)
{
	return os_atomic_ptr_cas((void *volatile *)ptr, expected, desired);
}
