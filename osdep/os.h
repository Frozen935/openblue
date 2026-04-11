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

/*
 * Unified public OS abstraction header.
 *
 * This header is the single include entry point for all modules.
 * It dispatches to the selected platform implementation based on macros.
 *
 * Default: POSIX. To use FreeRTOS, define OS_PLATFORM_FREERTOS.
 */
#ifndef OSDEP_OS_H
#define OSDEP_OS_H

/*
 * Platform selection:
 * - Define OS_PLATFORM_FREERTOS to include FreeRTOS backend.
 * - Define OS_PLATFORM_POSIX to explicitly select POSIX backend.
 * - Otherwise, POSIX backend is used by default.
 */
#if defined(OS_PLATFORM_FREERTOS)
#include "osdep/freertos/os.h"
#elif defined(OS_PLATFORM_POSIX)
#include "osdep/posix/os.h"
#else
#include "osdep/posix/os.h"
#endif

#endif /* OSDEP_OS_H */
