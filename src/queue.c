/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Simple circular queue implementation
 */

#include <spede/stdbool.h>
#include "queue.h"
#include "kernel.h"
#include <spede/stddef.h> // For NULL

/**
 * Initializes an empty queue
 * Sets the empty queue items to -1
 *
 * @param  queue - pointer to the queue
 * @return -1 on error; 0 on success
 */
int queue_init(queue_t *queue) {
    if(!queue) {
        kernel_log_error("Queue does not exist or is null.");
        return -1;
    }

    queue->head = queue->tail = -1;
    queue->size = 0;

    int i;
    for(i = 0; i < QUEUE_SIZE; i++) {
        queue->items[i] = -1;
    }
    return 0;
}

/**
 * Adds an item to the end of a queue
 * @param  queue - pointer to the queue
 * @param  item  - the item to add
 * @return -1 on error; 0 on success
 */
int queue_in(queue_t *queue, int item) {
    if(!queue) {
        kernel_log_error("queue: Unable to queue into uninitialized queue.");
        return -1;
    }

    if(queue_is_full(queue)) {
        kernel_log_error("Queue is full, item not added.");
        kernel_log_trace("Queue head: %i", queue->head);
        kernel_log_trace("Queue tail: %i", queue->tail);
        kernel_log_trace("Queue size: %i", queue->size);
        return -1;
    }

    if(queue_is_empty(queue)) {
        queue->head = 0;
    }
    // Mod (%) doesn't take effect until the tail reaches the QUEUE_SIZE.
    // This places the tail at element 0, following the circular queue specification.
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->items[queue->tail] = item;
    queue->size++;

    return 0;
}

/**
 * Pulls an item from the specified queue
 * @param  queue - pointer to the queue
 * @param  item  - pointer to the memory to save item to
 * @return -1 on error; 0 on success
 */
int queue_out(queue_t *queue, int *item) {
    if(queue_is_empty(queue)) {
        kernel_log_debug("Queue is empty; unable to dequeue.");
        return -1;
    }

    // Dequeues item.
    *item = queue->items[queue->head];

    // Clears element.
    queue->items[queue->head] = 0;

    // If it becomes empty, recreate queue.
    if(queue->head == queue->tail) {
        queue_init(queue);
    }
    else {
        queue->head = (queue->head + 1) % QUEUE_SIZE;
        queue->size--;
    }
    return 0;
}

/**
 * Indicates if the queue is empty
 * @param queue - pointer to the queue structure
 * @return true if empty, false if not empty
 */
bool queue_is_empty(queue_t *queue) {
    if(queue->size == 0) {
        return true;
    }
    return false;
}

/**
 * Indicates if the queue if full
 * @param queue - pointer to the queue structure
 * @return true if full, false if not full
 */
bool queue_is_full(queue_t *queue) {
    if(queue->size == QUEUE_SIZE) {
        return true;
    }
    return false;
}
