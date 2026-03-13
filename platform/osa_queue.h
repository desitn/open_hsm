/**
 * @file osa_queue.h
 * @brief OS Abstraction Layer - Message Queue
 *
 * Message queue abstraction interface compatible with typical RTOS APIs.
 * Platform-specific implementations are in platform/<platform>/ directories.
 */

#ifndef __OSA_QUEUE_H__
#define __OSA_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Message queue configuration */
#define OSA_QUEUE_NAME_LEN       32
#define OSA_QUEUE_DEFAULT_SIZE   20

/* Message queue handle - platform specific implementation */
typedef void* osa_queue_t;

/**
 * @brief Create a message queue
 * @param queue Pointer to store the created queue handle
 * @param name Queue name (for debugging)
 * @param msg_size Size of each message in bytes
 * @param max_msgs Maximum number of messages in queue
 * @return 0 on success, negative on error
 */
int osa_queue_create(osa_queue_t *queue, const char *name,
                     size_t msg_size, int max_msgs);

/**
 * @brief Destroy a message queue
 * @param queue Queue handle
 * @return 0 on success, negative on error
 */
int osa_queue_destroy(osa_queue_t queue);

/**
 * @brief Send a message to the queue
 * @param queue Queue handle
 * @param msg Pointer to message data
 * @param msg_len Length of message (must match msg_size)
 * @param timeout_ms Timeout in milliseconds (-1 for infinite, 0 for non-blocking)
 * @return 0 on success, negative on error or timeout
 */
int osa_queue_send(osa_queue_t queue, const void *msg,
                   size_t msg_len, int timeout_ms);

/**
 * @brief Receive a message from the queue
 * @param queue Queue handle
 * @param msg Pointer to store received message
 * @param msg_len Length of message buffer (must match msg_size)
 * @param timeout_ms Timeout in milliseconds (-1 for infinite, 0 for non-blocking)
 * @return 0 on success, negative on error or timeout
 */
int osa_queue_receive(osa_queue_t queue, void *msg,
                      size_t msg_len, int timeout_ms);

/**
 * @brief Get current message count in queue
 * @param queue Queue handle
 * @return Number of messages in queue, negative on error
 */
int osa_queue_count(osa_queue_t queue);

/**
 * @brief Check if queue is full
 * @param queue Queue handle
 * @return true if full, false otherwise
 */
bool osa_queue_is_full(osa_queue_t queue);

/**
 * @brief Check if queue is empty
 * @param queue Queue handle
 * @return true if empty, false otherwise
 */
bool osa_queue_is_empty(osa_queue_t queue);

/**
 * @brief Get queue name
 * @param queue Queue handle
 * @return Queue name string
 */
const char* osa_queue_get_name(osa_queue_t queue);

#ifdef __cplusplus
}
#endif

#endif /* __OSA_QUEUE_H__ */
