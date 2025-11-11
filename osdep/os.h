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
