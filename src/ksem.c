/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel Semaphores
 */

#include <spede/string.h>
#include <spede/stdbool.h>

#include "kernel.h"
#include "ksem.h"
#include "queue.h"
#include "scheduler.h"

// Table of all semephores
sem_t semaphores[SEM_MAX];

// semaphore ids to be allocated
queue_t sem_queue;

/**
 * Initializes kernel semaphore data structures
 * @return -1 on error, 0 on success
 */
int ksemaphores_init() {
    kernel_log_info("Initializing kernel semaphores");

    // Initialize the semaphore table
    for (int i = 0; i < SEM_MAX; i++) {
        semaphores[i].allocated = 0;
        semaphores[i].count = 0;
        queue_init(&semaphores[i].wait_queue);
    }

    // Initialize the semaphore queue
    queue_init(&sem_queue);

    // Fill the semaphore queue
    for (int i = 0; i < SEM_MAX; i++) {
        if(queue_in(&sem_queue, i) != 0) {
            kernel_log_error("ksem: Unable to fill the semaphore queue.");
            return -1;
        }
    }

    return 0;
}

/**
 * Allocates a semaphore
 * @param value - initial semaphore value
 * @return -1 on error, otherwise the semaphore id that was allocated
 */
int ksem_init(int value) {
    // Obtain a semaphore id from the semaphore queue
    int sem_id = -1;
    sem_t *sem;

    if (queue_out(&sem_queue, &sem_id) != 0) {
        kernel_log_error("ksem: Unable to obtain an ID from the semaphore queue.");
        return -1;
    }

    // Ensure that the id is within the valid range
    if (sem_id < 0 || sem_id >= SEM_MAX) {
        kernel_log_error("ksem: Semaphore ID not within valid range.");
        return -1;
    }

    // Initialize the semaphore data structure
    // sempohare table + all members (wait queue, allocated, count)
    // set count to initial value
    sem = &semaphores[sem_id];
    memset(sem, 0, sizeof(sem_t));
    sem->allocated = 1;
    sem->count = value;
    queue_init(&sem->wait_queue);

    return sem_id;
}

/**
 * Frees the specified semaphore
 * @param id - the semaphore id
 * @return 0 on success, -1 on error
 */
int ksem_destroy(int id) {
    sem_t *sem;

    if(id < 0 || id >= SEM_MAX) {
        kernel_log_error("ksem: Unable to destroy semaphore ID outside the valid range.");
        return -1;
    }

    // Look up the semaphore in the semaphore table.
    sem = &semaphores[id];

    if (sem->allocated == 0) {
        kernel_log_error("ksem: Could not destroy semaphore. Semaphore not allocated.");
        return -1;
    }

    // If the semaphore is locked, prevent it from being destroyed
    if (sem->count == 0) {
        kernel_log_error("ksem: Could not destroy semaphore. Semaphore locked.");
        return -1;
    }

    // Add the id back into the semaphore queue to be re-used later
    if(queue_in(&sem_queue, id) != 0) {
        kernel_log_error("ksem: Unable to queue in ID back into semaphore queue.");
        return -1;
    }

    // Clear the memory for the data structure
    free(sem);

    return 0;
}

/**
 * Waits on the specified semaphore if it is held
 * @param id - the semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksem_wait(int id) {
    sem_t *sem;

    if (id < 0 || id >= SEM_MAX) {
        kernel_log_error("ksem: Unable to wait for semaphore ID outside the valid range.");
        return -1;
    }

    // Look up the semaphore in the semaphore table.
    sem = &semaphores[id];

    if (sem->allocated == 0) {
        kernel_log_error("ksem: Could not wait on semaphore. Semaphore not allocated.");
        return -1;
    }

    // If the semaphore count is 0, then the process must wait
        // Set the state to WAITING
        // Add to the semaphore's wait queue
        // Remove from the scheduler
    if (sem->count == 0) {
        if(!active_proc) {
            kernel_log_error("ksem: Unable to change state of null active process.");
            return -1;
        }
        active_proc->state = WAITING;
        if(queue_in(&sem->wait_queue, active_proc->pid) != 0) {
            kernel_log_error("ksem: Unable to add process to the semaphore wait queue.");
            return -1;
        }
        scheduler_remove(active_proc);
        return 0;
    }

    // If the semaphore count is > 0
        // Decrement the count
    if (sem->count > 0) {
        sem->count--;
        return sem->count;
    }

    return -1;
}

/**
 * Posts the specified semaphore
 * @param id - the semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksem_post(int id) {
    sem_t *sem;
    int pid = -1;

    // Look up the semaphore in the semaphore table.
    sem = &semaphores[id];

    if (sem->allocated == 0) {
        kernel_log_error("ksem: Unable to post semaphore. Semaphore not allocated.");
        return -1;
    }

    // Incrememnt the semaphore count.
    sem->count++;

    // Check if any processes are waiting on the semaphore (semaphore wait queue)
        // If so, queue out and add to the scheduler
        // Decrement the semaphore count
    if (!queue_is_empty(&sem->wait_queue)) {
        if(queue_out(&sem->wait_queue, &pid) != 0) {
            kernel_log_error("ksem: Unable to obtain process from the semaphore wait queue.");
            return -1;
        }
        scheduler_add(pid_to_proc(pid));
        sem->count--;
        return sem->count;
    }

    return -1;
}
