/**
 * @file osa_sync.h
 * @brief OS Abstraction Layer - Synchronization Primitives
 *
 * Mutex and Semaphore abstraction interface compatible with typical RTOS APIs.
 * Platform-specific implementations are in platform/<platform>/ directories.
 */

#ifndef __OSA_SYNC_H__
#define __OSA_SYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Mutex handle - platform specific implementation */
typedef void* osa_mutex_t;

/* Semaphore handle - platform specific implementation */
typedef void* osa_sem_t;

/**
 * @brief Create a mutex
 *
 * @param[out] mutex    Output mutex handle
 *
 * @return 0 on success, negative error code on failure
 */
int osa_mutex_create(osa_mutex_t *mutex);

/**
 * @brief Destroy a mutex
 *
 * @param[in] mutex     Mutex handle to destroy
 */
void osa_mutex_destroy(osa_mutex_t mutex);

/**
 * @brief Lock a mutex
 *
 * @param[in] mutex     Mutex handle to lock
 */
void osa_mutex_lock(osa_mutex_t mutex);

/**
 * @brief Unlock a mutex
 *
 * @param[in] mutex     Mutex handle to unlock
 */
void osa_mutex_unlock(osa_mutex_t mutex);

/**
 * @brief Try to lock a mutex (non-blocking)
 *
 * @param[in] mutex     Mutex handle to try lock
 *
 * @return 0 on success (locked), -1 if already locked
 */
int osa_mutex_trylock(osa_mutex_t mutex);

/**
 * @brief Create a semaphore
 *
 * @param[out] sem          Output semaphore handle
 * @param[in]  initial_count    Initial count value
 * @param[in]  max_count        Maximum count value (0 for binary semaphore)
 *
 * @return 0 on success, negative error code on failure
 */
int osa_sem_create(osa_sem_t *sem, int initial_count, int max_count);

/**
 * @brief Destroy a semaphore
 *
 * @param[in] sem       Semaphore handle to destroy
 */
void osa_sem_destroy(osa_sem_t sem);

/**
 * @brief Wait on a semaphore
 *
 * @param[in] sem       Semaphore handle
 * @param[in] timeout_ms    Timeout in milliseconds
 *                          -1: wait forever
 *                           0: non-blocking
 *                          >0: wait up to timeout_ms milliseconds
 *
 * @return 0 on success, -ETIMEDOUT on timeout, other negative on error
 */
int osa_sem_wait(osa_sem_t sem, int timeout_ms);

/**
 * @brief Post/Signal a semaphore
 *
 * @param[in] sem       Semaphore handle to post
 */
void osa_sem_post(osa_sem_t sem);

/**
 * @brief Get current semaphore count
 *
 * @param[in] sem       Semaphore handle
 *
 * @return Current count value, negative on error
 */
int osa_sem_get_count(osa_sem_t sem);

#ifdef __cplusplus
}
#endif

#endif /* __OSA_SYNC_H__ */
