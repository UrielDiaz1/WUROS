/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel Process Handling
 */

#include <spede/string.h>
#include <spede/stdio.h>
#include <spede/time.h>
#include <spede/machine/proc_reg.h>
#include <spede/stdbool.h>

#include "kernel.h"
#include "kproc.h"
#include "scheduler.h"
#include "timer.h"

#include "queue.h"

// Process Queues
queue_t run_queue;
queue_t sleep_queue;

/**
 * Scheduler timer callback
 */
void scheduler_timer(void) {
    // Update active processes' cpu time and total run time.
    if(active_proc) {
        active_proc->run_time++;
        active_proc->cpu_time++;
    }

    // Decrement sleep_time for processes in sleep_queue, add back to run_queue
    // if necessary.
    if(!queue_is_empty(&sleep_queue)) {
        int pid;
        proc_t *proc;

        for(int i = 0; i < sleep_queue.size; i++) {
            // Queues out process from sleep queue and temporarily hold its pid.
            if(queue_out(&sleep_queue, &pid) != 0) {
                kernel_log_warn("scheduler: Unable to queue out id from sleep queue.");
                continue;
            }

            // Get the process' pointer by using the pid provided by the dequeue.
            proc = pid_to_proc(pid);
            if(!proc) {
                kernel_log_warn("scheduler: Unable to search process id %d", pid);
                continue;
            }

            // Wake the process and queue it back into the run queue.
            if(proc->sleep_time == 1) {
                proc->sleep_time = 0;
                scheduler_add(proc);
                //proc->state = IDLE;
                //queue_in(&run_queue, pid);
            }
            else {
                // Reduce the timer by one and queue it back into the sleep queue.
                proc->sleep_time--;
                queue_in(&sleep_queue, pid);
            }
        }
    }
}

/**
 * Executes the scheduler
 * Should ensure that `active_proc` is set to a valid process entry
 */
void scheduler_run(void) {
    int pid;

    // Ensure that processes not in the active state aren't still scheduled.
    if(active_proc && active_proc->state != ACTIVE) {
        active_proc = NULL;
    }

    // Check if we have an active process.
    if(active_proc) {
        // Check if the current process has exceeded it's time slice.
        if(active_proc->cpu_time >= SCHEDULER_TIMESLICE) {
            // Reset the active time.
            active_proc->cpu_time = 0;

            // If the process is not the idle task, add it back to the scheduler.
            if(active_proc->pid != 0) {
                scheduler_add(active_proc);
            }
            else {
                // Otherwise, simply set the state to IDLE.
                active_proc->state = IDLE;
            }

            // Unschedule the active process.
            active_proc = NULL;
        }
    }

    // Check if we have a process scheduled or not.
    if(!active_proc) {
        // Get the proces id from the run queue.
        if(queue_out(&run_queue, &pid) != 0) {
            // Default to process id 0 (idle task) if a process can't be scheduled.
            pid = 0;
        }
        // Update the active proc pointer.
        active_proc = pid_to_proc(pid);
        kernel_log_trace("Active proc set to proc pid[%d]", pid);
    }

    // Make sure we have a valid process at this point
    if(!active_proc) {
        kernel_panic("scheduler: There is no active valid process!");
    }

    // Ensure that the process state is set.
    active_proc->state = ACTIVE;
}

/**
 * Adds a process to the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_add(proc_t *proc) {
    if(!proc) {
        kernel_panic("scheduler: Unable to add invalid process to scheduler.");
    }

    // Prevents kernel process to be placed into the run queue.
    if(proc->pid == 0) {
        return;
    }

    // Add the process to the run queue.
    if(queue_in(&run_queue, proc->pid) != 0) {
        kernel_panic("scheduler: Unable to add the process to the scheduler.");
    }

    // Set the process state.
    proc->state = IDLE;
    proc->cpu_time = 0;
}

/**
 * Removes a process from the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_remove(proc_t *proc) {
    int pid;

    if(!proc) {
        kernel_log_debug("scheduler: Invalid process; no process was removed.");
        return;
    }

    // Iterate through each process in the queue.
    for(int i = 0; i < run_queue.size; i++) {

        // Dequeue each process, then check if its a match. If it matches, do not add
        // back into queue, otherwise, add it back in.
        if(queue_out(&run_queue, &pid) != 0) {
            kernel_log_warn("scheduler: Unable to queue out the process entry");
        }

        if(proc->pid == pid) {
            // The process is found.
            continue;
        }

        if(queue_in(&run_queue, pid) != 0) {
            kernel_panic("scheduler: Unable to queue process back to the run queue.");
        }
    }

    if(!active_proc) {
        return;
    }

    // If the process is the active process, ensure that the active process is
    // cleared so when the scheduler runs again, it will select a new process to run.
    if(proc->pid == active_proc->pid) {
        active_proc = NULL;
    }
}

/**
 * Puts a process to sleep.
 * @param proc    - pointer to the process entry.
 * @param seconds - number of seconds to sleep.
 */
void scheduler_sleep(proc_t *proc, int seconds) {
    if(!proc) {
        kernel_panic("scheduler: Unable to put invalid process to sleep.");
        return;
    }

    proc->sleep_time = seconds;

    // Check if already in the sleeping queue.
    if(proc->state == SLEEPING) {
        return;
    }

    scheduler_remove(proc);
    proc->state = SLEEPING;

    if(queue_in(&sleep_queue, proc->pid) != 0) {
        kernel_log_warn("scheduler: Unable to queue in process into sleep queue.");
    }
}

/**
 * Initializes the scheduler, data structures, etc.
 */
void scheduler_init(void) {
    kernel_log_info("Initializing scheduler");

    // Initialize any data structures or variables
    queue_init(&run_queue);
    queue_init(&sleep_queue);

    // Register the timer callback (scheduler_timer) to run every tick.
    timer_callback_register(&scheduler_timer, 1, -1);
}

