/**
 *
 *
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel Process Handling
 */

#include <spede/stdio.h>
#include <spede/string.h>
#include <spede/machine/proc_reg.h>

#include "kernel.h"
#include "trapframe.h"
#include "kproc.h"
#include "scheduler.h"
#include "timer.h"
#include "queue.h"
#include "vga.h"
#include "syscall_common.h"
#include "prog_user.h"
#include "tty.h"
#include "ringbuf.h"

// Next available process id to be assigned
int next_pid;

// Process table allocator
queue_t proc_allocator;

// Process table
proc_t proc_table[PROC_MAX];

// Process stacks
unsigned char proc_stack[PROC_MAX][PROC_STACK_SIZE];

// Active process
proc_t *active_proc;

// Function Declaration.
int kproc_attach_tty(int pid, int tty_index);

/**
 * Looks up a process in the process table via the process id
 * @param pid - process id
 * @return pointer to the process entry, NULL or error or if not found
 */
proc_t *pid_to_proc(int pid) {
    if(pid < 0 || pid >= next_pid) {
        kernel_log_error("PID %d is not assigned to any process.", pid);
        return NULL;
    }

    // Iterate through the process table and return a pointer to the valid entry
    // where the process id matches i.e. if proc_table[8].pid == pid, return
    // pointer to proc_table[8].
    // Ensure that the process control block actually refers to a valid process.
    for(int i = 0; i < PROC_MAX; i++) {
        // Checks for a valid PCB entry.
        if(proc_table[i].pid < 0) {
            continue;
        }
        else if(proc_table[i].pid == pid) {
          return &proc_table[i];
        }
    }
    kernel_log_debug("PID %d was not found in the process table.", pid);
    return NULL;
}

/**
 * Translates a process pointer to the entry index into the process table
 * @param proc - pointer to a process entry
 * @return the index into the process table, -1 on error
 */
int proc_to_entry(proc_t *proc) {
    if(!proc) {
        kernel_log_warn("proc_to_entry pointer parameter is NULL.");
        return -1;
    }

    // For a given process entry pointer, return the entry/index into the process table
    //  i.e. if proc -> proc_table[3], return 3
    // Ensure that the process control block actually refers to a valid process.
    for(int entry = 0; entry < PROC_MAX; entry++) {
        if(proc_table[entry].pid < 0) {
            continue;
        }
        else if(proc->pid == proc_table[entry].pid) {
            return entry;
        }
    }
    kernel_log_debug("Process with pid[%d] not found in the process table.", proc->pid);
    return -1;
}

/**
 * Returns a pointer to the given process entry
 */
proc_t * entry_to_proc(int entry) {
    if(entry < 0 || entry > PROC_MAX) {
        kernel_log_error("Entry %d is outside the scope of the process table.", entry);
        return NULL;
    }

    // For the given entry number, return a pointer to the process table entry
    // Ensure that the process control block actually refers to a valid process
    if(proc_table[entry].pid < 0) {
        return NULL;
    }
    else {
        return &proc_table[entry];
    }
}

/**
 * Creates a new process
 * @param proc_ptr - address of process to execute
 * @param proc_name - "friendly" process name
 * @param proc_type - process type (kernel or user)
 * @return process id of the created process, -1 on error
 */
int kproc_create(void *proc_ptr, char *proc_name, proc_type_t proc_type) {
    proc_t *proc = NULL;
    int ptable_entry = -1;

    // Allocate an entry in the process table via the process allocator
    if(queue_out(&proc_allocator, &ptable_entry) != 0) {
        kernel_log_warn("kproc: unable to allocate a process.");
        return -1;
    }

    // Initialize the process control block.
    proc = &proc_table[ptable_entry];

    // Initialize the process stack via proc_stack.
    proc->stack = &proc_stack[ptable_entry][PROC_STACK_SIZE];

    // Initialize the trapframe pointer at the bottom of the stack.
    proc->trapframe = (trapframe_t *)(&proc->stack[PROC_STACK_SIZE - sizeof(trapframe_t)]);

    // Set each of the process control block structure members to the initial starting values
    // as each new process is created, increment next_pid
    // proc->pid, state, type, run_time, cpu_time, start_time, etc.
    proc->pid        = next_pid++;
    proc->state      = IDLE;
    proc->type       = proc_type;
    proc->start_time = timer_get_ticks();
    proc->run_time   = 0;
    proc->cpu_time   = 0;
    proc->sleep_time = 0;
    proc->io[0]      = NULL;
    proc->io[1]      = NULL;

    // Copy the passed-in name to the name buffer in the process control block.
    if(strlen(proc_name) > PROC_NAME_LEN) {
        strcpy(proc->name, "DefaultUserName");
        kernel_log_warn("Name of process exceeds length by %d.", strlen(proc_name) - PROC_NAME_LEN);
    }
    else {
        strcpy(proc->name, proc_name);
    }

    // Set the instruction pointer in the trapframe.
    proc->trapframe->eip = (unsigned int)proc_ptr;

    // Set INTR flag
    proc->trapframe->eflags = EF_DEFAULT_VALUE | EF_INTR;

    // Set each segment in the trapframe.
    proc->trapframe->cs = get_cs();
    proc->trapframe->ds = get_ds();
    proc->trapframe->es = get_es();
    proc->trapframe->fs = get_fs();
    proc->trapframe->gs = get_gs();

    // Add the process to the scheduler
    scheduler_add(proc);

    kernel_log_info("Created process %s (%d) entry=%d", proc->name, proc->pid, ptable_entry);
    return proc->pid;
}

/**
 * Destroys a process
 * If the process is currently scheduled it must be unscheduled
 * @param proc - process control block
 * @return 0 on success, -1 on error
 */
int kproc_destroy(proc_t *proc) {
    if(!proc) {
        kernel_log_debug("Unable to destroy process. Process doesn't exist.");
        return -1;
    }

    // Prevents destruction of kernel process.
    if(proc->pid == 0) {
        kernel_log_warn("Unable to destroy process with pid[0].");
        return -1;
    }

    // Remove the process from the scheduler
    scheduler_remove(proc);

    // Clear/Reset all process data (process control block, stack, etc) related to the process
    int entry = proc_to_entry(proc);
    proc_table[entry].pid        = -1;
    proc_table[entry].state      = NONE;
    proc_table[entry].type       = PROC_TYPE_NONE;
    strcpy(proc_table[entry].name, "NULL");
    proc_table[entry].start_time = -1;
    proc_table[entry].run_time   = -1;
    proc_table[entry].cpu_time   = -1;
    proc_table[entry].sleep_time = 0;
    proc_table[entry].stack      = NULL;
    proc_table[entry].trapframe  = NULL;

    // Add the process entry/index value back into the process allocator
    if(queue_in(&proc_allocator, entry) != 0) {
        kernel_log_error("kproc: unable to queue proc entry back into process allocator.");
        return -1;
    }
    return 0;
}

/**
 * Idle Process
 */
void kproc_idle(void) {
    while (1) {
        // Ensure interrupts are enabled
        asm("sti");

        // Halt the CPU
        asm("hlt");
    }
}

/**
 * Test process
 */
void kproc_test(void) {
    // Loop forever
    while (1);
}


/**
 * Sets a process' input/output pointers to the specified TTY's
 * intput/output buffers.
 * @param pid       - The PID of the process to attach.
 * @param tty_index - The TTY index to attach to the process.
 * @return -1 on error, 0 when successful.
 */
int kproc_attach_tty(int pid, int tty_index) {
    proc_t *proc = pid_to_proc(pid);
    tty_t *tty = tty_get(tty_index);

    if(proc && tty) {
        kernel_log_info("Attaching process with PID[%d] to TTY[%d].", pid, tty_index);
        proc->io[PROC_IO_IN] = &tty->io_input;
        proc->io[PROC_IO_OUT] = &tty->io_output;
        return 0;
    }
    return -1;
}

/**
 * Initializes all process related data structures
 * Creates the first process (kernel_idle)
 * Registers the callback to display the process table/status
 */
void kproc_init(void) {
    kernel_log_info("Initializing process management");

    // Initialize all data structures and variables.
    next_pid = 0;

    // Initialize the process allocator queue.
    queue_init(&proc_allocator);

    // Populates items into the process allocator queue with the
    // indexes to be used for the process table.
    for(int ptable_entry = 0; ptable_entry < PROC_MAX; ptable_entry++) {
        if(!queue_is_full(&proc_allocator)) {
            queue_in(&proc_allocator, ptable_entry);
        }
        else {
            break;
        }
    }

    // Initialize the proc_table.
    for(int i = 0; i < PROC_MAX; i++) {
        proc_table[i].pid        = -1;
        proc_table[i].state      = NONE;
        proc_table[i].type       = PROC_TYPE_NONE;
        strcpy(proc_table[i].name, "NULL");
        proc_table[i].start_time = -1;
        proc_table[i].run_time   = -1;
        proc_table[i].cpu_time   = -1;
        proc_table[i].sleep_time = 0;
        proc_table[i].stack      = NULL;
        proc_table[i].trapframe  = NULL;
    }

    // Initialize the process stack.
    for(int i = 0; i < PROC_MAX; i++) {
        for(int j = 0; j < PROC_STACK_SIZE; j++) {
            proc_stack[i][j] = '\0';
        }
    }

    // Create the idle process (kproc_idle) as a kernel process.
    kproc_create(&kproc_idle, "idle", PROC_TYPE_KERNEL);

    int pid = -1;
    // Creates the shell processes.
    for(int m = 1; m < 5; m++) {
        // Creates a shell process. If successful, it returns
        // the process' PID.
        pid = kproc_create(&prog_shell, "shell", PROC_TYPE_USER);

        // If no errors occurred in the creation of the process,
        // inform the terminal.
        if(pid != -1) {
            kproc_attach_tty(pid, m);
        }
    }

    for (int i = 0; i < 3; i++) {
        pid = kproc_create(prog_ping, "ping", PROC_TYPE_USER);
        kernel_log_debug("Created ping process %d", pid);
        kproc_attach_tty(pid, (TTY_MAX - (pid % 2) - 1));
    }

    for (int i = 0; i < 3; i++) {
        pid = kproc_create(prog_pong, "pong", PROC_TYPE_USER);
        kernel_log_debug("Created pong process %d", pid);
        kproc_attach_tty(pid, (TTY_MAX - (pid % 2) - 1));
    }

}

