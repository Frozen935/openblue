/*
 * Copyright (C) 2026
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
