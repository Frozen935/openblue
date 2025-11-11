#ifndef __INCLUDE_TOOLCHAIN_H__
#define __INCLUDE_TOOLCHAIN_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BUILD_ASSERT(EXPR, MSG...) _Static_assert(EXPR, "" MSG)

#ifndef __noinit
#define __noinit /* no-op */
#endif

#ifndef __must_check
#define __must_check __attribute__((warn_unused_result))
#endif

#ifndef __deprecated
#define __deprecated __attribute__((deprecated))
#endif

#ifndef __maybe_unused
#define __maybe_unused __attribute__((__unused__))
#endif

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x) __attribute__((aligned(x)))
#endif

#ifndef __weak
#define __weak __attribute__((weak))
#endif

#ifndef __alignof__
#define __alignof__(t) _Alignof(t)
#endif

#define ARG_UNUSED(arg) (void)(arg)

#endif /* __INCLUDE_TOOLCHAIN_H__ */