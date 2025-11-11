#ifndef __BASE_ATOMIC_H__
#define __BASE_ATOMIC_H__

typedef unsigned long atomic_t;
typedef unsigned long atomic_val_t;

/* Atomic pointer placeholder */
#ifndef atomic_ptr_t
typedef void *atomic_ptr_t;
#endif
#ifndef atomic_ptr_val_t
typedef atomic_ptr_t atomic_ptr_val_t;
#endif

#define ATOMIC_BITS            (sizeof(atomic_val_t) * BITS_PER_BYTE)
#define ATOMIC_MASK(bit)       BIT((unsigned long)(bit) & (ATOMIC_BITS - 1U))
#define ATOMIC_ELEM(addr, bit) ((addr) + ((bit) / ATOMIC_BITS))

#define ATOMIC_BITMAP_SIZE(num_bits) (ROUND_UP(num_bits, ATOMIC_BITS) / ATOMIC_BITS)

#define ATOMIC_DEFINE(name, num_bits) atomic_t name[ATOMIC_BITMAP_SIZE(num_bits)]

/* Compare-and-swap */
bool atomic_cas(atomic_t *target, atomic_val_t expected, atomic_val_t desired);

/* Prototypes implemented in shim_atomic.c */
atomic_val_t atomic_get(const atomic_t *target);
atomic_val_t atomic_set(atomic_t *target, atomic_val_t value);
atomic_val_t atomic_inc(atomic_t *target);
atomic_val_t atomic_dec(atomic_t *target);
void atomic_add(atomic_t *target, atomic_val_t value);
void atomic_sub(atomic_t *target, atomic_val_t value);

static inline void atomic_clear(atomic_t *target)
{
	atomic_set(target, (atomic_val_t)0);
}

bool atomic_test_bit(const atomic_t *target, int bit);
void atomic_set_bit(atomic_t *target, int bit);
void atomic_clear_bit(atomic_t *target, int bit);
void atomic_set_bit_to(atomic_t *target, int bit, bool val);
bool atomic_test_and_set_bit(atomic_t *target, int bit);
bool atomic_test_and_clear_bit(atomic_t *target, int bit);

/* atomic pointer helpers (compile-only prototypes) */
void *atomic_ptr_get(atomic_ptr_t *ptr);
void *atomic_ptr_set(atomic_ptr_t *ptr, void *val);
void *atomic_ptr_clear(atomic_ptr_t *ptr);
bool atomic_ptr_cas(atomic_ptr_t *ptr, void *expected, void *desired);

#endif /* __BASE_ATOMIC_H__ */