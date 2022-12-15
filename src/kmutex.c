/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel Mutexes
 */

#include <spede/string.h>

#include "kernel.h"
#include "kmutex.h"
#include "queue.h"
#include "scheduler.h"
#include "kproc.h"

// Table of all mutexes
mutex_t mutexes[MUTEX_MAX];

// Mutex ids to be allocated
queue_t mutex_queue;

/**
 * Initializes kernel mutex data structures
 * @return -1 on error, 0 on success
 */
int kmutexes_init() {
    kernel_log_info("Initializing kernel mutexes");

    // Initialize the mutex table.
    for(int i = 0; i < MUTEX_MAX; i++) {
        mutexes[i].allocated = 0;
        mutexes[i].locks = 0;
        mutexes[i].owner = NULL;
        queue_init(&mutexes[i].wait_queue);
    }

    // Initialize the mutex queue.
    queue_init(&mutex_queue);

    // Fill the mutex queue
    for(int i = 0; i < MUTEX_MAX; i++) {
        if(queue_in(&mutex_queue, i) != 0) {
            kernel_log_error("kmutex: Unable to fill the mutex queue.");
            return -1;
        }
    }

    return 0;
}

/**
 * Allocates a mutex
 * @return -1 on error, otherwise the mutex id that was allocated
 */
int kmutex_init(void) {
    int id = -1;
    mutex_t *mutex;

    // Obtain a mutex id from the mutex queue.
    if(queue_out(&mutex_queue, &id) != 0) {
        kernel_log_error("kmutex: Unable to obtain an ID from the mutex queue.");
        return -1;
    }

    // Ensure that the id is within the valid range.
    if(id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: Obtained ID is outside a valid range.");
        return -1;
    }

    // Pointer to the mutex table entry.
    mutex = &mutexes[id];

    // Initialize the mutex data structure (mutex_t + all members).
    memset(mutex, 0, sizeof(mutex_t));

    //Initialize wait_queue.
    queue_init(&mutex->wait_queue);

    // Allocated member should be set to 1.
    mutex->allocated = 1;

    // Return the mutex id.
    return id;
}

/**
 * Frees the specified mutex
 * @param id - the mutex id
 * @return 0 on success, -1 on error
 */
int kmutex_destroy(int id) {
    mutex_t *mutex;

    // Ensure that the id is within the valid range.
    if(id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: Unable to destroy mutex ID outside the valid range.");
        return -1;
    }

    // Look up the mutex in the mutex table.
    mutex = &mutexes[id];

    // If the mutex is locked, prevent it from being destroyed (return error).
    if(mutex->locks > 0) {
        kernel_log_error("kmutex: Unable to destroy locked mutex.");
        return -1;
    }

    // Add the id back into the mutex queue to be re-used later.
    if(queue_in(&mutex_queue, id) != 0) {
        kernel_log_error("kmutex: Unable to queue in id back into mutex queue.");
        return -1;
    }

    // Clear the memory for the data structure.
    free(mutex);

    return 0;
}

/**
 * Locks the specified mutex
 * @param id - the mutex id
 * @return -1 on error, otherwise the current lock count
 */
int kmutex_lock(int id) {
    mutex_t *mutex;

    // Ensure that the id is within the valid range.
    if(id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: Unable to lock mutex ID outside the valid range.");
        return -1;
    }

    // Look up the mutex in the mutex table.
    mutex = &mutexes[id];

    // If the mutex is already locked
    //   1. Set the active process state to WAITING
    //   2. Add the process to the mutex wait queue (so it can take
    //      the mutex when it is unlocked)
    //   3. Remove the process from the scheduler, allow another
    //      process to be scheduled

    if(mutex->owner) {
        if(!active_proc) {
            kernel_log_error("kmutex: Unable to enter null active process into wait queue.");
            return -1;
        }
        active_proc -> state = WAITING;

        if(queue_in(&mutex -> wait_queue, active_proc -> pid) != 0) {
            kernel_log_error("kmutex: Unable to add process to the mutex wait queue");
            return -1;
        }

        scheduler_remove(active_proc);
    }
    // If the mutex is not locked
    //   1. Set the mutex owner to the active process
    else {
        mutex -> owner = active_proc;
    }

    // Increment the lock count.
    mutex->locks++;

    // Return the mutex lock count.
    return mutex->locks;
}

/**
 * Unlocks the specified mutex
 * @param id - the mutex id
 * @return -1 on error, otherwise the current lock count
 */
int kmutex_unlock(int id) {
    mutex_t *mutex;
    proc_t *proc;
    int pid = -1;

    if(id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: Unable to unlock mutex ID outside valid range.");
        return -1;
    }

    // Look up the mutex in the mutex table.
    mutex = &mutexes[id];

    // If the mutex is not locked, there is nothing to do.
    if(!mutex->owner) {
        return 0;
    }

    // Decrement the lock count.
    mutex->locks--;

    // If there are no more locks held:
    //    1. Clear the owner of the mutex
    if(mutex->locks == 0) {
        mutex->owner = NULL;
    }
    // If there are still locks held:
    //    1. Obtain a process from the mutex wait queue
    //    2. Add the process back to the scheduler
    //    3. Set the owner of the mutex to the process
    else {
        if(queue_out(&mutex->wait_queue, &pid) != 0) {
            kernel_log_error("kmutex: Unable to obtain process from the mutex wait queue.");
            return -1;
        }
        proc = pid_to_proc(pid);
        scheduler_add(proc);
        mutex->owner = proc;
    }

    // Return the mutex lock count.
    return mutex->locks;
}
