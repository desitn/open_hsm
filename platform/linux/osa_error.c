/**
 * @file osa_error.c
 * @brief OS Abstraction Layer - Error Code Implementation (Linux)
 *
 * Error code to string conversion implementation.
 */

#include "../osa_error.h"
#include <string.h>

/**
 * @brief Convert error code to string
 */
const char* osa_error_string(int err)
{
    switch (err) {
        /* Success */
        case OSA_OK:
            return "Success";

        /* General errors */
        case OSA_ERR_GENERIC:
            return "Generic error";
        case OSA_ERR_INVALID_PARAM:
            return "Invalid parameter";
        case OSA_ERR_NULL_POINTER:
            return "Null pointer";
        case OSA_ERR_NOT_INITIALIZED:
            return "Not initialized";
        case OSA_ERR_ALREADY_INITIALIZED:
            return "Already initialized";
        case OSA_ERR_NOT_IMPLEMENTED:
            return "Not implemented";
        case OSA_ERR_NOT_SUPPORTED:
            return "Not supported";
        case OSA_ERR_BUSY:
            return "Resource busy";
        case OSA_ERR_TIMEOUT:
            return "Operation timeout";
        case OSA_ERR_CANCELED:
            return "Operation canceled";
        case OSA_ERR_ABORTED:
            return "Operation aborted";

        /* Memory/Resource errors */
        case OSA_ERR_NO_MEMORY:
            return "Out of memory";
        case OSA_ERR_NO_RESOURCES:
            return "No resources available";
        case OSA_ERR_RESOURCE_BUSY:
            return "Resource busy/in use";
        case OSA_ERR_RESOURCE_EXHAUSTED:
            return "Resource exhausted";

        /* Synchronization errors */
        case OSA_ERR_MUTEX_LOCK:
            return "Mutex lock failed";
        case OSA_ERR_MUTEX_UNLOCK:
            return "Mutex unlock failed";
        case OSA_ERR_SEM_WAIT:
            return "Semaphore wait failed";
        case OSA_ERR_SEM_POST:
            return "Semaphore post failed";
        case OSA_ERR_SEM_TIMEOUT:
            return "Semaphore timeout";

        /* Thread/Task errors */
        case OSA_ERR_THREAD_CREATE:
            return "Thread creation failed";
        case OSA_ERR_THREAD_JOIN:
            return "Thread join failed";
        case OSA_ERR_THREAD_DETACH:
            return "Thread detach failed";
        case OSA_ERR_THREAD_CANCEL:
            return "Thread cancel failed";
        case OSA_ERR_THREAD_NOT_FOUND:
            return "Thread not found";

        /* Queue/Message errors */
        case OSA_ERR_QUEUE_CREATE:
            return "Queue creation failed";
        case OSA_ERR_QUEUE_FULL:
            return "Queue full";
        case OSA_ERR_QUEUE_EMPTY:
            return "Queue empty";
        case OSA_ERR_QUEUE_SEND:
            return "Queue send failed";
        case OSA_ERR_QUEUE_RECV:
            return "Queue receive failed";
        case OSA_ERR_QUEUE_TIMEOUT:
            return "Queue operation timeout";

        /* Worker pool errors */
        case OSA_ERR_WORKER_INIT:
            return "Worker pool init failed";
        case OSA_ERR_WORKER_START:
            return "Worker pool start failed";
        case OSA_ERR_WORKER_STOP:
            return "Worker pool stop failed";
        case OSA_ERR_WORKER_SUBMIT:
            return "Job submit failed";
        case OSA_ERR_WORKER_CANCEL:
            return "Job cancel failed";
        case OSA_ERR_WORKER_WAIT:
            return "Job wait failed";
        case OSA_ERR_WORKER_QUEUE_FULL:
            return "Worker queue full";
        case OSA_ERR_WORKER_NOT_FOUND:
            return "Worker/job not found";
        case OSA_ERR_WORKER_BUSY:
            return "Worker busy";
        case OSA_ERR_WORKER_SHUTDOWN:
            return "Worker pool shutting down";

        /* HSM errors */
        case OSA_ERR_HSM_INIT:
            return "HSM init failed";
        case OSA_ERR_HSM_START:
            return "HSM start failed";
        case OSA_ERR_HSM_STOP:
            return "HSM stop failed";
        case OSA_ERR_HSM_INVALID_STATE:
            return "Invalid HSM state";
        case OSA_ERR_HSM_INVALID_EVENT:
            return "Invalid HSM event";
        case OSA_ERR_HSM_INVALID_SIGNAL:
            return "Invalid HSM signal";
        case OSA_ERR_HSM_SEND_FAILED:
            return "HSM send event failed";
        case OSA_ERR_HSM_NOT_FOUND:
            return "HSM not found";

        /* Platform specific errors */
        case OSA_ERR_PLATFORM_BASE:
            return "Platform error";

        default:
            if (err < OSA_ERR_PLATFORM_BASE) {
                return "Unknown platform error";
            }
            return "Unknown error";
    }
}

/**
 * @brief Get error code category
 */
const char* osa_error_category(int err)
{
    if (err == OSA_OK) {
        return "Success";
    }
    if (err >= -99) {
        return "General";
    }
    if (err >= -199) {
        return "Memory/Resource";
    }
    if (err >= -299) {
        return "Synchronization";
    }
    if (err >= -399) {
        return "Thread/Task";
    }
    if (err >= -499) {
        return "Queue/Message";
    }
    if (err >= -599) {
        return "Worker Pool";
    }
    if (err >= -699) {
        return "HSM";
    }
    if (err <= OSA_ERR_PLATFORM_BASE) {
        return "Platform";
    }
    return "Unknown";
}
