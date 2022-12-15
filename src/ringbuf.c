/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Simple ring buffer implementation
 */

#include <spede/stdbool.h>      // for bool type
#include <spede/stddef.h>       // for size_t
#include "ringbuf.h"
#include "kernel.h"

/**
 * Initializes an empty ring buffer
 * Sets the empty queue items to -1
 *
 * @param  buf - pointer to the ring buffer data structure
 * @return -1 on error; 0 on success
 */
int ringbuf_init(ringbuf_t *buf) {
    if(buf == NULL) {
        kernel_log_error("Null Buffer.");
        return -1;
    }
    buf->head = buf->tail = -1;
    buf->size = 0;

    int i;
    for(i = 0; i < RINGBUF_SIZE; i++) {
        buf->data[i] = 0;
    }
    return 0;
}

/**
 * Writes a byte to the buffer
 * @param  buf   - pointer to the ring buffer structure
 * @param  byte  - the byte to write
 * @return -1 on error; 0 on success
 */
int ringbuf_write(ringbuf_t *buf, char byte) {
    if(ringbuf_is_full(buf)) {
        kernel_log_trace("Buffer is full; unable to write.");
        return -1;
    }

    if(ringbuf_is_empty(buf)) {
        buf->head = 0;
    }
    buf->tail = (buf->tail + 1) % RINGBUF_SIZE;
    buf->data[buf->tail] = byte;
    buf->size++;
    return 0;
}

/**
 * Reads a byte from the buffer
 * @param  buf - pointer to the ring buffer structure
 * @param  byte - pointer to the byte to be stored
 * @return -1 on error; 0 on success
 */
int ringbuf_read(ringbuf_t *buf, char *byte) {
    if(ringbuf_is_empty(buf)) {
        kernel_log_trace("Buffer is empty; unable to read.");
        return -1;
    }

    *byte = buf->data[buf->head];

    // If read item was the last, clear the buffer.
    if(buf->head == buf->tail) {
        ringbuf_init(buf);
    }
    else {
        buf->head = (buf->head + 1) % RINGBUF_SIZE;
        buf->size--;
    }
    return 0;
}

/**
 * Copies multiple bytes to the buffer from the specified memory
 * @param buf - pointer to the ring buffer structure
 * @param mem - pointer to the memory location to copy from
 * @param size - number of bytes to copy
 * @return -1 on error, 0 on success
 * @note Should return an error if the number of bytes
 *       cannot be copied - i.e. the buffer would overflow
 */
int ringbuf_write_mem(ringbuf_t *buf, char *mem, size_t size) {
    // Comparing int with size_t could create false positives.
    if((int)size > (RINGBUF_SIZE - buf->size)) {
        kernel_log_info("Size of bytes exceed remaining buffer space");
        kernel_log_info("Size: %d, RINGBUF_SIZE: %d, Buffer->size: %d", (int)size, RINGBUF_SIZE, buf->size);
        return -1;
    }

    if(!buf) {
        kernel_log_error("Ringbuffer doesn't exist.");
    }

    if(ringbuf_is_empty(buf)) {
        buf->head = 0;
    }

    size_t i;
    for(i = 0; i < size; i++) {
        buf->data[buf->tail] = mem[i];
        kernel_log_info("Writing [%c]", mem[i]);
        kernel_log_info("Before increasing size: %d", buf->size);
        buf->size++;
        kernel_log_info("Increasing size to %d", buf->size);
        buf->tail = (buf->tail + 1) % RINGBUF_SIZE;
    }
    return 0;
}

/**
 * Copies multiple bytes from the buffer to the specified memory
 * @param buf - pointer to the ring buffer structure
 * @param mem - pointer to the memory location to copy to
 * @param size - number of bytes to copy
 * @return -1 on error, positive value to indicate number of bytes
 *         copied
 */
int ringbuf_read_mem(ringbuf_t *buf, char *mem, size_t size) {
    if(size > (size_t)buf->size) {
        return -1;
    }

    size_t i;
    for(i = 0; i < size; i++) {
        mem[i] = buf->data[buf->head];
        buf->head = (buf->head + 1) % RINGBUF_SIZE;
        buf->size--;
    }

    if(size == 0) {
        ringbuf_init(buf);
    }
    return 0;
}

/**
 * Flushes (empties) the buffer
 * @param buf - pointer to the ring buffer structure
 * @return -1 on error, 0 on success
 */
int ringbuf_flush(ringbuf_t *buf) {
    if(buf == NULL) {
        kernel_log_trace("Buffer doesn't exist; unable to flush.");
        return -1;
    }

    ringbuf_init(buf);
    return 0;
}

/**
 * Indicates if the buffer is empty
 * @param buf - pointer to the ring buffer structure
 * @return true if empty, false if not empty
 */
bool ringbuf_is_empty(ringbuf_t *buf) {
    if(buf->head == -1) {
        return true;
    }
    return false;
}

/**
 * Indicates if the buffer if full
 * @param buf - pointer to the ring buffer structure
 * @return true if full, false if not full
 */
bool ringbuf_is_full(ringbuf_t *buf) {
    if(buf->size == RINGBUF_SIZE) {
        return true;
    }
    return false;
}
