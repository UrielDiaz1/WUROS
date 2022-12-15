/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * System call APIs
 */
#include "syscall.h"
#include "kernel.h"

/**
 * Executes a system call without any arguments
 * @param syscall - the system call identifier
 * @return return code from the the system call
 */
int _syscall0(int syscall) {
    int rc = -1;

    // We need a way to get the syscall identifier to the kernel so that
    // our kernel handler knows which system call we're performing.
    // To do this, we pass a value through a register,
    //          "movl %1, %%eax;"
    // In here we have two operands %0 and %1. The %1 is our syscall.
    // The %0 is the rc variable.
    // So, we move the syscal (%1) into the eax register.
    //
    //
    // The second step is to perform an interrupt.
    //          "int $0x80;"
    // The $0x80 is a software defined interrupt. We're going to associate
    // this interrupt with our system calls. This means that every time
    // we do a system call, this interrupt will get triggered. Once this
    // interrupt gets triggered, a context switch occurs, where the kernel
    // starts performing the relevant operations to the system call. In the
    // meantime, this process just waits for the kernel to finish and context
    // switch back to the process.
    //
    // Once the kernel finishes and we return back to the process, we continue
    // with the third instruction.
    //          "movl %%eax, %0;"
    // Here, we move the content inside the eax register into "rc" (%0).
    // The content inside the eax register will be whatever the system call
    // requested from the kernel, and therefore, what the kernel placed inside it.
    //
    // Analogy: You have a basket. You leave a list of items inside the basket and take
    // it to the store. You leave the basket on the door and wait. The store owner
    // comes out, grabs the basket, and then reads the letter. He grabs all the items
    // in the letter and places them into the basket. Once finished, he leaves
    // the basket at the door. You then pick up the basket and leave with all
    // the items requested.
    // The basket is "eax", the letter is our system call, the store owner is
    // the kernel, and we are the process.
    //
    // The 3 following lines are a way to inform the compiler what we're trying
    // to do. The first two lines just tell the compiler we will store what's
    // in eax (where syscall was originally stored) into "rc".
    // The last line
    //          "%eax");
    // Informs the compiler that we will use this specific register. It is
    // important to inform the compiler so it doesn't use it for something else;
    // overriding its content before the instruction was able to take out
    // the stored data in eax.
    asm("movl %1, %%eax;"
        "int $0x80;"
        "movl %%eax, %0;"
        : "=g"(rc)
        : "g"(syscall)
        : "%eax");

    return rc;
}

/**
 * Executes a system call with one argument
 * @param syscall - the system call identifier
 * @param arg1 - first argument
 * @return return code from the the system call
 */
int _syscall1(int syscall, int arg1) {
    int rc = -1;

    /// Same procedure as syscall0, but with two arguments.
    // %0 is the rc variable, %1 is the syscall, and %2 is arg1.
    asm("movl %1, %%eax;"
        "movl %2, %%ebx;"
        "int $0x80;"
        "movl %%eax, %0;"
        : "=g"(rc)
        : "g"(syscall), "g"(arg1)
        : "%eax", "%ebx");

    return rc;
}

/**
 * Executes a system call with two arguments
 * @param syscall - the system call identifier
 * @param arg1 - first argument
 * @param arg2 - second argument
 * @return return code from the the system call
 */
int _syscall2(int syscall, int arg1, int arg2) {
    int rc = -1;

    asm("movl %1, %%eax;"
        "movl %2, %%ebx;"
        "movl %3, %%ecx;"
        "int $0x80;"
        "movl %%eax, %0;"
        : "=g"(rc)
        : "g"(syscall), "g"(arg1), "g"(arg2)
        : "%eax", "%ebx", "%ecx");

    return rc;
}

/**
 * Executes a system call with three arguments
 * @param syscall - the system call identifier
 * @param arg1 - first argument
 * @param arg2 - second argument
 * @param arg3 - third argument
 * @return return code from the the system call
 */
int _syscall3(int syscall, int arg1, int arg2, int arg3) {
    int rc = -1;

    asm("movl %1, %%eax;"
        "movl %2, %%ebx;"
        "movl %3, %%ecx;"
        "movl %4, %%edx;"
        "int $0x80;"
        "movl %%eax, %0;"
        : "=g"(rc)
        : "g"(syscall), "g"(arg1), "g"(arg2), "g"(arg3)
        : "%eax", "%ebx", "%ecx", "%edx");

    return rc;
}

/**
 * Gets the current system time (in seconds)
 * @return system time in seconds
 */
int sys_get_time(void) {
    return _syscall0(SYSCALL_SYS_GET_TIME);
}

/**
 * Gets the operating system name
 * @param name - pointer to a character buffer where the name will be copied
 * @return 0 on success, -1 or other non-zero value on error
 */
int sys_get_name(char *name) {
    // The syscall functions only move around integers. So,
    // to be able to move around the name pointer, we need to typecast it
    // into an integer and then include it as an argument.
    //
    // This action doesn't cause issues because our OS is 32-bits,
    // the pointer is 32-bits, and integers are 32-bits. There is no
    // mismatch in the bits themselves.
    //
    // However, once the kernel uses the typecasted pointer and returns it, we
    // need to typecast it back into its original data type.
    return _syscall1(SYSCALL_SYS_GET_NAME, (int)name);
}

/**
 * Puts the current process to sleep for the specified number of seconds
 * @param seconds - number of seconds the process should sleep
 */
void proc_sleep(int secs) {
    _syscall1(SYSCALL_PROC_SLEEP, secs);
}

/**
 * Exits the current process
 * @param exitcode An exit code to return to the parent process
 */
void proc_exit(int exitcode) {
    _syscall1(SYSCALL_PROC_EXIT, exitcode);
}

/**
 * Gets the current process' id
 * @return process id
 */
int proc_get_pid(void) {
    return _syscall0(SYSCALL_PROC_GET_PID);
}

/**
 * Gets the current process' name
 * @param name - pointer to a character buffer where the name will be copied
 * @return 0 on success, -1 or other non-zero value on error
 */
int proc_get_name(char *name) {
    return _syscall1(SYSCALL_PROC_GET_NAME, (int)name);
}

/**
 * Writes up to n bytes to the process' specified IO buffer
 * @param io - the IO buffer to write to
 * @param buf - the buffer to copy from
 * @param n - number of bytes to write
 * @return -1 on error or value indicating number of bytes copied
 */
int io_write(int io, char *buf, int n) {
    return _syscall3(SYSCALL_IO_WRITE, io, (int)buf, n);
}

/**
 * Reads up to n bytes from the process' specified IO buffer
 * @param io - the IO buffer to read from
 * @param buf - the buffer to copy to
 * @param n - number of bytes to read
 * @return -1 on error or value indicating number of bytes copied
 */
int io_read(int io, char *buf, int n) {
    return _syscall3(SYSCALL_IO_READ, io, (int)buf, n);
}

/**
 * Flushes (clears) the specified IO buffer
 * @param io - the IO buffer to flush
 * @return -1 on error or 0 on success
 */
int io_flush(int io) {
    return _syscall1(SYSCALL_IO_FLUSH, io);
}

/**
 * Allocates a mutex from the kernel
 * @return -1 on error, all other values indicate the mutex id
 */
int mutex_init(void){
    return _syscall0(SYSCALL_MUTEX_INIT);
}

/**
 * Detroys a mutex
 * @return -1 on error, 0 on sucecss
 */
int mutex_destroy(int mutex){
    return _syscall1(SYSCALL_MUTEX_DESTROY, mutex);
}

/**
 * Locks the mutex
 * @param mutex - mutex id
 * @return -1 on error, 0 on sucecss
 * @note If the mutex is already locked, process will block/wait.
 */
int mutex_lock(int mutex){
    return _syscall1(SYSCALL_MUTEX_LOCK, mutex);
}

/**
 * Unlocks the mutex
 * @param mutex - mutex id
 * @return -1 on error, 0 on sucecss
 */
int mutex_unlock(int mutex){
    return _syscall1(SYSCALL_MUTEX_UNLOCK, mutex);
}

/**
 * Allocates a semaphore from the kernel
 * @param value - initial semaphore value
 * @return -1 on error, all other values indicate the semaphore id
 */
int sem_init(int value){
    return _syscall1(SYSCALL_SEM_INIT, value);
}

/**
 * Destroys a semaphore
 * @param sem - semaphore id
 * @return -1 on error, 0 on success
 */
int sem_destroy(int sem){
    return _syscall1(SYSCALL_SEM_DESTROY, sem);
}

/**
 * Waits on a semaphore
 * @param sem - semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int sem_wait(int sem){
    return _syscall1(SYSCALL_SEM_WAIT, sem);
}

/**
 * Posts a semaphore
 * @param sem - semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int sem_post(int sem){
    return _syscall1(SYSCALL_SEM_POST, sem);
}
