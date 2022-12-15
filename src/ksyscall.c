/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel System Call Handlers
 */
#include <spede/time.h>
#include <spede/string.h>
#include <spede/stdio.h>

#include "kernel.h"
#include "kproc.h"
#include "ksyscall.h"
#include "interrupts.h"
#include "scheduler.h"
#include "timer.h"
#include "ringbuf.h"
#include "kmutex.h"
#include "ksem.h"

/**
 * System call IRQ handler
 * Dispatches system calls to the function associate with the specified system call
 */
void ksyscall_irq_handler(void) {
    // Default return value.
    int rc = -1;

    // System call number.
    int syscall;

    // Arguments.
    unsigned int arg1;
    unsigned int arg2;
    unsigned int arg3;

    if (!active_proc) {
        kernel_panic("ksyscall: Invalid process.");
    }

    if (!active_proc->trapframe) {
        kernel_panic("ksyscall: Invalid trapframe.");
    }

    // System call identifier is stored on the EAX register.
    // Additional arguments should be stored on additional registers (EBX, ECX, etc.)
    syscall = active_proc->trapframe->eax;
    arg1 = active_proc->trapframe->ebx;
    arg2 = active_proc->trapframe->ecx;
    arg3 = active_proc->trapframe->edx;

    // Based upon the system call identifier, call the respective system call handler.
    //
    // Ensure that the EAX register for the active process contains the return value.
    if (active_proc->trapframe->eax == SYSCALL_NONE) {
        kernel_log_warn("ksyscall: No specific system call was invoked.");
        return;
    }

    switch (syscall) {
        // The following parameters are stored in the respective registers:
        // trapframe->ebx = int io    - the IO buffer to read from.
        // trapframe->ecx = char *buf - the buffer to copy to.
        // trapframe->edx = int n     - number of bytes to read.
        // All parameters that are not integers must be casted to the
        // appropriate data type.
        case SYSCALL_IO_READ:
            rc = ksyscall_io_read(arg1, (char *)arg2, arg3);
            break;

        // The following parameters are stored in the respective registers:
        // trapframe->ebx = int io    - the IO buffer to write to.
        // trapframe->ecx = char *buf - the buffer to copy from.
        // trapframe->edx = int n     - number of bytes to write.
        // All parameters that are not integers must be casted to the
        // appropriate data type.
        case SYSCALL_IO_WRITE:
            rc = ksyscall_io_write(arg1, (char *)arg2, arg3);
            break;

        // The following parameter is stored in the respective register:
        // trapframe->ebx = int io - the IO buffer to flush.
        case SYSCALL_IO_FLUSH:
            rc = ksyscall_io_flush(arg1);
            break;

        // This syscall has no parameters. It just returns the time.
        case SYSCALL_SYS_GET_TIME:
            rc = ksyscall_sys_get_time();
            break;

        // The following parameter is stored in the respective register:
        // trapframe->ebx = char *name - pointer to the character buffer
        //                               where the name will be copied to.
        // The parameter must be cast as a char pointer, as it is currently int.
        case SYSCALL_SYS_GET_NAME:
            rc = ksyscall_sys_get_name((char *)arg1);
            break;

        // The following parameter is stored in the respective register:
        // trapframe->ebx = int seconds - number of seconds the process
        //                                should sleep.
        case SYSCALL_PROC_SLEEP:
            rc = ksyscall_proc_sleep(arg1);
            break;

        // This syscall has no parameters. It returns the success or failure of the task.
        case SYSCALL_PROC_EXIT:
            rc = ksyscall_proc_exit();
            break;

        // This syscall has no parameters. It returns the current process' id.
        case SYSCALL_PROC_GET_PID:
            rc = ksyscall_proc_get_pid();
            break;

        // The following parameter is stored in the respective register:
        // trapframe->ebx = char *name - pointer to a character buffer where the
        //                               name will be copied to.
        case SYSCALL_PROC_GET_NAME:
            rc = ksyscall_proc_get_name((char *)arg1);
            break;

        // This syscall has no parameters. It allocates a mutex.
        case SYSCALL_MUTEX_INIT:
            rc = ksyscall_mutex_init();
            break;

        // This following parameter is stored in the respective register:
        // trapframe->ebx = int mutex - the mutex id to destroy
        case SYSCALL_MUTEX_DESTROY:
            rc = ksyscall_mutex_destroy(arg1);
            break;

        // This following parameter is stored in the respective register:
        // trapframe->ebx = int mutex - the mutex id to lock
        case SYSCALL_MUTEX_LOCK:
            rc = ksyscall_mutex_lock(arg1);
            break;

        // This following parameter is stored in the respective register:
        // trapframe->ebx = int mutex - the mutex id to unlock
        case SYSCALL_MUTEX_UNLOCK:
            rc = ksyscall_mutex_unlock(arg1);
            break;

        // This following parameter is stored in the respective register:
        // trapframe->ebx = int sem - the semaphore id to allocate
        case SYSCALL_SEM_INIT:
            rc = ksyscall_sem_init(arg1);
            break;

        // This following parameter is stored in the respective register:
        // trapframe->ebx = int sem - the semaphore id to destroy
        case SYSCALL_SEM_DESTROY:
            rc = ksyscall_sem_destroy(arg1);
            break;

        // This following parameter is stored in the respective register:
        // trapframe->ebx = int sem - the semaphore id that waits for post
        case SYSCALL_SEM_WAIT:
            rc = ksyscall_sem_wait(arg1);
            break;

        // This following parameter is stored in the respective register:
        // trapframe->ebx = int sem - the semaphore id to post
        case SYSCALL_SEM_POST:
            rc = ksyscall_sem_post(arg1);
            break;
        default:
            kernel_panic("kysyscall: Invalid system call %d!", syscall);
    }

    // Returns a value, if appropriate, into the EAX register.
    if(active_proc) {
        active_proc->trapframe->eax = (unsigned int)rc;
    }
}

/**
 * System Call Initialization
 */
void ksyscall_init(void) {
    kernel_log_info("Initializing System Call");

    // Register the IDT entry and IRQ handler for the syscall IRQ (IRQ_SYSCALL)
    interrupts_irq_register(IRQ_SYSCALL, isr_entry_syscall, ksyscall_irq_handler);
}

/**
 * Writes up to n bytes to the process' specified IO buffer
 * @param io - the IO buffer to write to
 * @param buf - the buffer to copy from
 * @param n - number of bytes to write
 * @return -1 on error or value indicating number of bytes copied
 */
int ksyscall_io_write(int io, char *buf, int size) {
    if(io < 0 || io > 1){
        kernel_log_error("ksyscall: Invalid write buffer.");
        return -1;
    }

    if(!buf){
        kernel_log_error("ksyscall: No buffer to copy from.");
        return -1;
    }

    if(size < 0){
        kernel_log_error("ksyscall: Can't write %d bytes", size);
        return -1;
    }

    if(!active_proc) {
        kernel_log_error("ksyscall: No active process to work with.");
        return -1;
    }

    if(!active_proc->io[io]) {
        kernel_log_error("ksyscall: Unable to write into null io buffer.");
        return -1;
    }

    int i;
    for(i = 0; i < size; i++) {
        ringbuf_write(active_proc->io[io], buf[i]);
    }
    return size;
}

/**
 * Reads up to n bytes from the process' specified IO buffer
 * @param io - the IO buffer to read from
 * @param buf - the buffer to copy to
 * @param n - number of bytes to read
 * @return -1 on error or value indicating number of bytes copied
 */
int ksyscall_io_read(int io, char *buf, int size) {
    if(io < 0 || io > 1){
        kernel_log_error("ksyscall: invalid read buffer.");
        return -1;
    }

    if(!buf){
        kernel_log_error("ksyscall: no buffer to copy from");
        return -1;
    }

    if(size < 0){
        kernel_log_error("ksyscall: can't write %d bytes", size);
        return -1;
    }

    if(!active_proc->io[io]) {
        kernel_log_error("ksyscall: Unable to read null io buffer.");
        return -1;
    }

    if(!active_proc) {
        kernel_log_error("ksyscall: no active process to work with.");
    }

    // Limits the amount of characters read by the size of the buffer.
    int s = active_proc->io[io]->size;
    if(s > size) {
        s = size;
    }

    int i;
    char c = '\0';
    // Read one character at a time from the io buffer.
    for(i = 0; i < size; i++) {
        ringbuf_read(active_proc->io[io], &c);
        buf[i] = c;
    }
    // Ensure io buffer is empty and ready to take in the new inputs.
    ksyscall_io_flush(io);
    return s;
}

/**
 * Flushes (clears) the specified IO buffer
 * @param io - the IO buffer to flush
 * @return -1 on error or 0 on success
 */
int ksyscall_io_flush(int io) {
    if(!active_proc) {
        kernel_log_error("ksyscall: Unable to flush null active process.");
        return -1;
    }

    if(io < 0 || io > PROC_IO_MAX){
        kernel_log_error("ksyscall: can't flush invalid buffer.");
        return -1;
    }

    if(!active_proc->io[io]) {
        kernel_log_error("ksyscall: Unable to flush null io buffer.");
        return -1;
    }

    ringbuf_flush(active_proc->io[io]);
    return 0;
}

/**
 * Gets the current system time (in seconds)
 * @return system time in seconds
 */
int ksyscall_sys_get_time(void) {
    return timer_get_ticks() / 100;
}

/**
 * Gets the operating system name
 * @param name - pointer to a character buffer where the name will be copied
 * @return 0 on success, -1 or other non-zero value on error
 */
int ksyscall_sys_get_name(char *name) {
    if(!name) {
        kernel_log_error("ksyscall: no buffer to read OS name.");
        return -1;
    }

    strncpy(name, OS_NAME, sizeof(OS_NAME));
    return 0;
}

/**
 * Puts the active process to sleep for the specified number of seconds
 * @param seconds - number of seconds the process should sleep
 */
int ksyscall_proc_sleep(int seconds) {
    if(seconds < 0) {
        kernel_log_error("ksyscall: Invalid sleep time.");
        return -1;
    }

    scheduler_sleep(active_proc, seconds * 100);
    return 0;
}

/**
 * Exits the current process
 */
int ksyscall_proc_exit(void) {
    if(!active_proc) {
        kernel_log_error("ksyscall: No active process to exit.");
        return -1;
    }

    return kproc_destroy(active_proc);
}

/**
 * Gets the active process pid
 * @return process id or -1 on error
 */
int ksyscall_proc_get_pid(void) {
    if(!active_proc) {
        kernel_log_error("ksyscall: Unable to get PID from null active process.");
        return -1;
    }

    return active_proc->pid;
}

/**
 * Gets the active process' name
 * @param name - pointer to a character buffer where the name will be copied
 * @return 0 on success, -1 or other non-zero value on error
 */
int ksyscall_proc_get_name(char *name) {
    if(!active_proc) {
        kernel_log_error("ksyscall: Unable to get name of null active process.");
        return -1;
    }

    if(!name) {
        kernel_log_error("ksyscall: Invalid name.");
        return -1;
    }

    strncpy(name, active_proc->name, PROC_NAME_LEN);
    return 0;
}

/**
 * Allocates a mutex from the kernel
 * @return -1 on error, all other values indicate the mutex id
 */
int ksyscall_mutex_init() {
    return kmutex_init();
}

/**
 * Detroys a mutex
 * @return -1 on error, 0 on sucecss
 */
int ksyscall_mutex_destroy(int mutex) {
    if(mutex < 0 || mutex > MUTEX_MAX){
        kernel_log_error("ksyscall: Can't destroy out of bounds mutex ID.");
        return -1;
    }

    return kmutex_destroy(mutex);
}

/**
 * Locks the mutex
 * @param mutex - mutex id
 * @return -1 on error, 0 on sucecss
 * @note If the mutex is already locked, process will block/wait.
 */
int ksyscall_mutex_lock(int mutex) {
    if(mutex < 0 || mutex > MUTEX_MAX){
        kernel_log_error("ksyscall: Can't lock out of bounds mutex ID.");
        return -1;
    }

    return kmutex_lock(mutex);
}

/**
 * Unlocks the mutex
 * @param mutex - mutex id
 * @return -1 on error, 0 on sucecss
 */
int ksyscall_mutex_unlock(int mutex) {
    if(mutex < 0 || mutex > MUTEX_MAX){
        kernel_log_error("ksyscall: Can't unlock out of bounds mutex ID.");
        return -1;
    }

    return kmutex_unlock(mutex);
}

/**
 * Allocates / creates a semaphore from the kernel
 * @param value - initial semaphore value
 * @return -1 on error, otherwise the semaphore id that was allocated
 */
int ksyscall_sem_init(int value){
    if(value < 0 || value > SEM_MAX){
        kernel_log_error("ksyscall: Iniitial semaphore value out of bounds.");
        return -1;
    }

    return ksem_init(value);
}

/**
 * Destroys a semaphore
 * @param sem - semaphore id
 * @return -1 on error, 0 on success
 */
int ksyscall_sem_destroy(int sem){
    if(sem < 0 || sem > SEM_MAX){
        kernel_log_error("ksyscall: Can't destroy out of bounds Semaphore ID.");
        return -1;
    }

    return ksem_destroy(sem);
}

/**
 * Waits on a semaphore
 * @param sem - semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksyscall_sem_wait(int sem){
    if(sem < 0 || sem > SEM_MAX){
        kernel_log_error("ksyscall: Can't wait on out of bounds Semaphore ID.");
        return -1;
    }

    return ksem_wait(sem);
}

/**
 * Posts a semaphore
 * @param sem - semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksyscall_sem_post(int sem){
    if(sem < 0 || sem > SEM_MAX){
        kernel_log_error("ksyscall: Can't post out of bounds Semaphore ID.");
        return -1;
    }

    return ksem_post(sem);
}
