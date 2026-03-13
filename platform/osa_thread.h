/**
 * @file osa_thread.h
 * @brief OS Abstraction Layer - Thread Interface
 *
 * Thread abstraction interface compatible with typical RTOS APIs.
 * Platform-specific implementations are in platform/<platform>/ directories.
 */

#ifndef __OSA_THREAD_H__
#define __OSA_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Thread handle - platform specific implementation */
typedef void* osa_thread_t;

/* Thread creation parameters - RTOS style */
struct osa_thread_param {
    const char *name;        /**< Thread name */
    int stack_size;          /**< Stack size in bytes */
    int priority;            /**< Thread priority (platform specific range) */
    void *stack_buf;         /**< Stack buffer (NULL = dynamic allocation) */
};

/* Thread entry function type */
typedef void (*osa_thread_entry_t)(void *param);

/**
 * @brief Create a new thread
 *
 * @param[out] thread   Output thread handle
 * @param[in]  param    Thread creation parameters
 * @param[in]  entry    Thread entry function
 * @param[in]  arg      Argument passed to entry function
 *
 * @return 0 on success, negative error code on failure
 */
int osa_thread_create(osa_thread_t *thread,
                      const struct osa_thread_param *param,
                      osa_thread_entry_t entry,
                      void *arg);

/**
 * @brief Delete a thread
 *
 * @param[in] thread    Thread handle to delete
 *
 * @return 0 on success, negative error code on failure
 */
int osa_thread_delete(osa_thread_t thread);

/**
 * @brief Exit current thread (called from within thread)
 */
void osa_thread_exit(void);

/**
 * @brief Suspend a thread
 *
 * @param[in] thread    Thread handle to suspend
 *
 * @return 0 on success, negative error code on failure
 */
int osa_thread_suspend(osa_thread_t thread);

/**
 * @brief Resume a suspended thread
 *
 * @param[in] thread    Thread handle to resume
 *
 * @return 0 on success, negative error code on failure
 */
int osa_thread_resume(osa_thread_t thread);

/**
 * @brief Get current thread handle
 *
 * @return Current thread handle, NULL if not in thread context
 */
osa_thread_t osa_thread_self(void);

/**
 * @brief Sleep for specified milliseconds
 *
 * @param[in] ms        Sleep time in milliseconds
 */
void osa_thread_sleep(uint32_t ms);

/**
 * @brief Yield CPU to other threads
 */
void osa_thread_yield(void);

/**
 * @brief Wait for a thread to terminate (join)
 *
 * @param[in] thread    Thread handle to wait for
 *
 * @return 0 on success, negative error code on failure
 */
int osa_thread_join(osa_thread_t thread);

#ifdef __cplusplus
}
#endif

#endif /* __OSA_THREAD_H__ */
