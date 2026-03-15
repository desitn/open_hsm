/**
 * @file osa_error.h
 * @brief OS Abstraction Layer - Error Codes
 *
 * Unified error code definitions and error string conversion functions.
 * All OSA functions should return these error codes instead of raw integers.
 */

#ifndef OSA_ERROR_H
#define OSA_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief OSA error codes
 *
 * Error codes are negative values, 0 means success.
 * Range: -1 to -99: General errors
 * Range: -100 to -199: Memory/Resource errors
 * Range: -200 to -299: Synchronization errors
 * Range: -300 to -399: Thread/Task errors
 * Range: -400 to -499: Queue/Message errors
 * Range: -500 to -599: Worker pool errors
 * Range: -600 to -699: HSM errors
 */
enum osa_error_code {
    /* Success */
    OSA_OK = 0,                     /**< Success */

    /* General errors (-1 to -99) */
    OSA_ERR_GENERIC = -1,           /**< Generic error */
    OSA_ERR_INVALID_PARAM = -2,     /**< Invalid parameter */
    OSA_ERR_NULL_POINTER = -3,      /**< Null pointer */
    OSA_ERR_NOT_INITIALIZED = -4,   /**< Not initialized */
    OSA_ERR_ALREADY_INITIALIZED = -5, /**< Already initialized */
    OSA_ERR_NOT_IMPLEMENTED = -6,   /**< Not implemented */
    OSA_ERR_NOT_SUPPORTED = -7,     /**< Not supported */
    OSA_ERR_BUSY = -8,              /**< Resource busy */
    OSA_ERR_TIMEOUT = -9,           /**< Operation timeout */
    OSA_ERR_CANCELED = -10,         /**< Operation canceled */
    OSA_ERR_ABORTED = -11,          /**< Operation aborted */

    /* Memory/Resource errors (-100 to -199) */
    OSA_ERR_NO_MEMORY = -100,       /**< Out of memory */
    OSA_ERR_NO_RESOURCES = -101,    /**< No resources available */
    OSA_ERR_RESOURCE_BUSY = -102,   /**< Resource busy/in use */
    OSA_ERR_RESOURCE_EXHAUSTED = -103, /**< Resource exhausted */

    /* Synchronization errors (-200 to -299) */
    OSA_ERR_MUTEX_LOCK = -200,      /**< Mutex lock failed */
    OSA_ERR_MUTEX_UNLOCK = -201,    /**< Mutex unlock failed */
    OSA_ERR_SEM_WAIT = -210,        /**< Semaphore wait failed */
    OSA_ERR_SEM_POST = -211,        /**< Semaphore post failed */
    OSA_ERR_SEM_TIMEOUT = -212,     /**< Semaphore timeout */

    /* Thread/Task errors (-300 to -399) */
    OSA_ERR_THREAD_CREATE = -300,   /**< Thread creation failed */
    OSA_ERR_THREAD_JOIN = -301,     /**< Thread join failed */
    OSA_ERR_THREAD_DETACH = -302,   /**< Thread detach failed */
    OSA_ERR_THREAD_CANCEL = -303,   /**< Thread cancel failed */
    OSA_ERR_THREAD_NOT_FOUND = -304, /**< Thread not found */

    /* Queue/Message errors (-400 to -499) */
    OSA_ERR_QUEUE_CREATE = -400,    /**< Queue creation failed */
    OSA_ERR_QUEUE_FULL = -401,      /**< Queue full */
    OSA_ERR_QUEUE_EMPTY = -402,     /**< Queue empty */
    OSA_ERR_QUEUE_SEND = -403,      /**< Queue send failed */
    OSA_ERR_QUEUE_RECV = -404,      /**< Queue receive failed */
    OSA_ERR_QUEUE_TIMEOUT = -405,   /**< Queue operation timeout */

    /* Worker pool errors (-500 to -599) */
    OSA_ERR_WORKER_INIT = -500,     /**< Worker pool init failed */
    OSA_ERR_WORKER_START = -501,    /**< Worker pool start failed */
    OSA_ERR_WORKER_STOP = -502,     /**< Worker pool stop failed */
    OSA_ERR_WORKER_SUBMIT = -503,   /**< Job submit failed */
    OSA_ERR_WORKER_CANCEL = -504,   /**< Job cancel failed */
    OSA_ERR_WORKER_WAIT = -505,     /**< Job wait failed */
    OSA_ERR_WORKER_QUEUE_FULL = -506, /**< Worker queue full */
    OSA_ERR_WORKER_NOT_FOUND = -507, /**< Worker/job not found */
    OSA_ERR_WORKER_BUSY = -508,     /**< Worker busy */
    OSA_ERR_WORKER_SHUTDOWN = -509, /**< Worker pool shutting down */

    /* HSM errors (-600 to -699) */
    OSA_ERR_HSM_INIT = -600,        /**< HSM init failed */
    OSA_ERR_HSM_START = -601,       /**< HSM start failed */
    OSA_ERR_HSM_STOP = -602,        /**< HSM stop failed */
    OSA_ERR_HSM_INVALID_STATE = -603, /**< Invalid HSM state */
    OSA_ERR_HSM_INVALID_EVENT = -604, /**< Invalid HSM event */
    OSA_ERR_HSM_INVALID_SIGNAL = -605, /**< Invalid HSM signal */
    OSA_ERR_HSM_SEND_FAILED = -606, /**< HSM send event failed */
    OSA_ERR_HSM_NOT_FOUND = -607,   /**< HSM not found */

    /* Platform specific errors (-1000 to -1999) */
    OSA_ERR_PLATFORM_BASE = -1000,  /**< Platform error base */
};

/**
 * @brief Convert error code to string
 *
 * @param[in] err   Error code
 *
 * @return Error string description
 */
const char* osa_error_string(int err);

/**
 * @brief Check if error code indicates success
 *
 * @param[in] err   Error code
 *
 * @return true if success (err == OSA_OK), false otherwise
 */
static inline bool osa_is_ok(int err)
{
    return err == OSA_OK;
}

/**
 * @brief Check if error code indicates failure
 *
 * @param[in] err   Error code
 *
 * @return true if error (err < OSA_OK), false otherwise
 */
static inline bool osa_is_err(int err)
{
    return err < OSA_OK;
}

/**
 * @brief Get error code category
 *
 * @param[in] err   Error code
 *
 * @return Error category string
 */
const char* osa_error_category(int err);

#ifdef __cplusplus
}
#endif

#endif /* OSA_ERROR_H */
