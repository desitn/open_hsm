/**
 * @file osa_platform.h
 * @brief Platform Abstraction Layer - Main Interface
 *
 * This is the main header file for the OS Abstraction Layer (OSA).
 * It provides a unified interface for:
 * - Thread management
 * - Synchronization primitives (mutex, semaphore)
 * - Message queues
 * - Timers
 *
 * The abstraction layer is designed to be compatible with typical RTOS APIs
 * while providing a consistent interface across different platforms.
 *
 * @section Platform Selection
 * The platform is auto-detected based on predefined macros:
 * - __linux__  -> Linux platform
 * - FREERTOS   -> RTOS platform
 * - Default    -> Linux platform (for development)
 *
 * @section Usage
 * Include this header to access all platform abstraction features:
 * @code
 * #include "osa_platform.h"
 * @endcode
 *
 * Platform-specific implementations are in platform/<platform>/ directories.
 */

#ifndef __OSA_PLATFORM_H__
#define __OSA_PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Platform auto-detection
 *
 * Automatically selects the target platform based on compiler-defined macros.
 * Can be overridden by manually defining OSA_PLATFORM_LINUX or OSA_PLATFORM_RTOS.
 */
#if defined(__linux__)
    #define OSA_PLATFORM_LINUX
#elif defined(FREERTOS)
    #define OSA_PLATFORM_RTOS
#else
    /* Default to Linux for development */
    #define OSA_PLATFORM_LINUX
#endif

/* Include platform abstraction interfaces */
#include "osa_thread.h"     /**< Thread management */
#include "osa_sync.h"       /**< Synchronization primitives */
#include "osa_queue.h"      /**< Message queues */
#include "osa_timer.h"      /**< Timers */


#ifdef __cplusplus
}
#endif

#endif /* __OSA_PLATFORM_H__ */
