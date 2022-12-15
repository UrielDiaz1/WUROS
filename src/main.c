/**
 * CPE/CSC 159 Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Operating system entry point
 */

#include "interrupts.h"
#include "kernel.h"
#include "keyboard.h"
#include "kproc.h"
#include "timer.h"
#include "tty.h"
#include "scheduler.h"
#include "vga.h"
#include "ksyscall.h"
#include "kmutex.h"
#include "ksem.h"
#include "test.h"

int main(void) {

    // Always iniialize the kernel
    kernel_init();

    // Initialize interrupts
    interrupts_init();

    // Initialize timers
    timer_init();

    // Initialize the TTY
    tty_init();

    // Initialize the VGA driver
    vga_init();

    // Initialize the keyboard driver
    keyboard_init();

    // Scheduler initialization
    scheduler_init();

    // Initialize process management.
    kproc_init();

    // Test initialization
    test_init();

    // Kernel System Calls Initialization.
    ksyscall_init();

    // Mutexes initialization.
    kmutexes_init();

    // Semaphores initialization.
    ksemaphores_init();

    // Print a welcome message
    vga_printf("Welcome to %s!\n", OS_NAME);
    vga_puts("Press a key to continue...\n");

    // Wait for a key to be pressed
    keyboard_getc();

    // Clear the screen
    vga_clear();

    // Enable interrupts
    interrupts_enable();

    // Loop in place forever
    while (1);

    // Should never get here!
    return 0;
}
