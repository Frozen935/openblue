#ifndef __INCLUDE_UTIL_MACRO_H__
#define __INCLUDE_UTIL_MACRO_H__

#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC 1000U
#endif
#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC 1000U
#endif
#ifndef SEC_PER_MIN
#define SEC_PER_MIN 60U
#endif
#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000U
#endif
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000U
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

#ifndef BIT64
#define BIT64(_n) (1ULL << (_n))
#endif

#ifndef BIT_MASK
#define BIT_MASK(n) (BIT(n) - 1UL)
#endif

#ifndef BIT64_MASK
#define BIT64_MASK(n) (BIT64(n) - 1ULL)
#endif

#define BITS_PER_LONG      (BITS_PER_BYTE * sizeof(long))
#define BITS_PER_LONG_LONG (BITS_PER_BYTE * sizeof(long long))

#define GENMASK(h, l) (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#ifndef WRITE_BIT
#define WRITE_BIT(var, bit, set) ((var) = (set) ? ((var) | BIT(bit)) : ((var) & ~BIT(bit)))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#ifndef IN_RANGE
#define IN_RANGE(val, min, max) ((val) >= (min) && (val) <= (max))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

#define _XXXX1 _YYYY,

#define __IS_ENABLED3(ignore_this, val, ...) val
#define __IS_ENABLED2(one_or_two_args)       __IS_ENABLED3(one_or_two_args 1, 0)
#define __IS_ENABLED1(config_macro)          __IS_ENABLED2(_XXXX##config_macro)
#define IS_ENABLED(config_macro)             __IS_ENABLED1(config_macro)

#define __DEBRACKET(...)                            __VA_ARGS__
#define __GET_ARG2_DEBRACKET(ignore_this, val, ...) __DEBRACKET val
#define __COND_CODE_X(one_or_two_args, _if_code, _else_code)                                       \
	__GET_ARG2_DEBRACKET(one_or_two_args _if_code, _else_code)
#define __COND_CODE_1(_flag, _if_1_code, _else_code)                                               \
	__COND_CODE_X(_XXXX##_flag, _if_1_code, _else_code)

#define UTIL_COND_CODE(_flag, _if_1_code, _else_code) __COND_CODE_1(_flag, _if_1_code, _else_code)

#define IF_ENABLED(_flag, _code)  UTIL_COND_CODE(_flag, _code, ())
#define IF_DISABLED(_flag, _code) UTIL_COND_CODE(_flag, (), _code)

#define __FIELD_LSB_GET(value)  ((value) & -(value))
#define FIELD_GET(mask, value)  (((value) & (mask)) / __FIELD_LSB_GET(mask))
#define FIELD_PREP(mask, value) (((value) * __FIELD_LSB_GET(mask)) & (mask))

#define __UTIL_PRIMITIVE_CAT(a, ...)   a##__VA_ARGS__
#define __UTIL_CAT(a, ...)             __UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_LISTIFY(LEN, F, sep, ...) __UTIL_CAT(UTIL_LISTIFY_, LEN)(F, sep, __VA_ARGS__)

#define UTIL_LISTIFY_0(F, sep, ...)
#define UTIL_LISTIFY_1(F, sep, ...)  F(0, __VA_ARGS__)
#define UTIL_LISTIFY_2(F, sep, ...)  UTIL_LISTIFY_1(F, sep, __VA_ARGS__) __DEBRACKET sep F(1, __VA_ARGS__)
#define UTIL_LISTIFY_3(F, sep, ...)  UTIL_LISTIFY_2(F, sep, __VA_ARGS__) __DEBRACKET sep F(2, __VA_ARGS__)
#define UTIL_LISTIFY_4(F, sep, ...)  UTIL_LISTIFY_3(F, sep, __VA_ARGS__) __DEBRACKET sep F(3, __VA_ARGS__)
#define UTIL_LISTIFY_5(F, sep, ...)  UTIL_LISTIFY_4(F, sep, __VA_ARGS__) __DEBRACKET sep F(4, __VA_ARGS__)
#define UTIL_LISTIFY_6(F, sep, ...)  UTIL_LISTIFY_5(F, sep, __VA_ARGS__) __DEBRACKET sep F(5, __VA_ARGS__)
#define UTIL_LISTIFY_7(F, sep, ...)  UTIL_LISTIFY_6(F, sep, __VA_ARGS__) __DEBRACKET sep F(6, __VA_ARGS__)
#define UTIL_LISTIFY_8(F, sep, ...)  UTIL_LISTIFY_7(F, sep, __VA_ARGS__) __DEBRACKET sep F(7, __VA_ARGS__)
#define UTIL_LISTIFY_9(F, sep, ...)  UTIL_LISTIFY_8(F, sep, __VA_ARGS__) __DEBRACKET sep F(8, __VA_ARGS__)
#define UTIL_LISTIFY_10(F, sep, ...) UTIL_LISTIFY_9(F, sep, __VA_ARGS__) __DEBRACKET sep F(9, __VA_ARGS__)
#define UTIL_LISTIFY_11(F, sep, ...) UTIL_LISTIFY_10(F, sep, __VA_ARGS__) __DEBRACKET sep F(10, __VA_ARGS__)
#define UTIL_LISTIFY_12(F, sep, ...) UTIL_LISTIFY_11(F, sep, __VA_ARGS__) __DEBRACKET sep F(11, __VA_ARGS__)
#define UTIL_LISTIFY_13(F, sep, ...) UTIL_LISTIFY_12(F, sep, __VA_ARGS__) __DEBRACKET sep F(12, __VA_ARGS__)
#define UTIL_LISTIFY_14(F, sep, ...) UTIL_LISTIFY_13(F, sep, __VA_ARGS__) __DEBRACKET sep F(13, __VA_ARGS__)
#define UTIL_LISTIFY_15(F, sep, ...) UTIL_LISTIFY_14(F, sep, __VA_ARGS__) __DEBRACKET sep F(14, __VA_ARGS__)
#define UTIL_LISTIFY_16(F, sep, ...) UTIL_LISTIFY_15(F, sep, __VA_ARGS__) __DEBRACKET sep F(15, __VA_ARGS__)
#define UTIL_LISTIFY_17(F, sep, ...) UTIL_LISTIFY_16(F, sep, __VA_ARGS__) __DEBRACKET sep F(16, __VA_ARGS__)
#define UTIL_LISTIFY_18(F, sep, ...) UTIL_LISTIFY_17(F, sep, __VA_ARGS__) __DEBRACKET sep F(17, __VA_ARGS__)
#define UTIL_LISTIFY_19(F, sep, ...) UTIL_LISTIFY_18(F, sep, __VA_ARGS__) __DEBRACKET sep F(18, __VA_ARGS__)
#define UTIL_LISTIFY_20(F, sep, ...) UTIL_LISTIFY_19(F, sep, __VA_ARGS__) __DEBRACKET sep F(19, __VA_ARGS__)

#define __NUM_VA_ARGS_IMPL( \
	 _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
	 _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#define NUM_VA_ARGS(...) \
	__NUM_VA_ARGS_IMPL(__VA_ARGS__, \
		20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
		10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define FOR_EACH_1(F, sep, x) F(x)
#define FOR_EACH_2(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_1(F, sep, __VA_ARGS__)
#define FOR_EACH_3(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_2(F, sep, __VA_ARGS__)
#define FOR_EACH_4(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_3(F, sep, __VA_ARGS__)
#define FOR_EACH_5(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_4(F, sep, __VA_ARGS__)
#define FOR_EACH_6(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_5(F, sep, __VA_ARGS__)
#define FOR_EACH_7(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_6(F, sep, __VA_ARGS__)
#define FOR_EACH_8(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_7(F, sep, __VA_ARGS__)
#define FOR_EACH_9(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_8(F, sep, __VA_ARGS__)
#define FOR_EACH_10(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_9(F, sep, __VA_ARGS__)
#define FOR_EACH_11(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_10(F, sep, __VA_ARGS__)
#define FOR_EACH_12(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_11(F, sep, __VA_ARGS__)
#define FOR_EACH_13(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_12(F, sep, __VA_ARGS__)
#define FOR_EACH_14(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_13(F, sep, __VA_ARGS__)
#define FOR_EACH_15(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_14(F, sep, __VA_ARGS__)
#define FOR_EACH_16(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_15(F, sep, __VA_ARGS__)
#define FOR_EACH_17(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_16(F, sep, __VA_ARGS__)
#define FOR_EACH_18(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_17(F, sep, __VA_ARGS__)
#define FOR_EACH_19(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_18(F, sep, __VA_ARGS__)
#define FOR_EACH_20(F, sep, x, ...) F(x) __DEBRACKET sep FOR_EACH_19(F, sep, __VA_ARGS__)

#define FOR_EACH(F, sep, ...) __UTIL_CAT(FOR_EACH_, NUM_VA_ARGS(__VA_ARGS__))(F, sep, __VA_ARGS__)

#define REVERSE_ARGS(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) \
	a16, a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1

#define __UTIL_DO_CONCAT(x, y) x##y
#define UTIL_CONCAT(x, y)      __UTIL_DO_CONCAT(x, y)

#ifndef _CONCAT
#define _CONCAT(x, y) UTIL_CONCAT(x, y)
#endif

#define __UTILS_STRINGIFY(x) #x
#define UTILS_STRINGIFY(s) __UTILS_STRINGIFY(s)

#endif /* __INCLUDE_UTIL_MACRO_H__ */
