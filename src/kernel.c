/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel functions
 */
#include <spede/flames.h>
#include <spede/stdarg.h>
#include <spede/stdio.h>
#include <spede/string.h>

#include "kernel.h"
#include "vga.h"
#include "scheduler.h"
#include "interrupts.h"

#ifndef KERNEL_LOG_LEVEL_DEFAULT
#define KERNEL_LOG_LEVEL_DEFAULT KERNEL_LOG_LEVEL_INFO
#endif

// Current log level
int kernel_log_level = KERNEL_LOG_LEVEL_DEFAULT;

/**
 * Initializes any kernel internal data structures and variables
 */
void kernel_init() {
    // Display a welcome message on the host
    kernel_log_info("Welcome to %s!", OS_NAME);

    kernel_log_info("Initializing kernel...");

}

/**
 * Prints a kernel log message to the host with an error log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_error(char *msg, ...) {
    if (kernel_log_level < KERNEL_LOG_LEVEL_ERROR) {
        return;
    }

    // Obtain the list of variable arguments
    va_list args;

    // Indicate this is an 'error' type of message
    printf("error: ");

    // Pass the message variable arguments to vprintf
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a warning log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_warn(char *msg, ...) {
    if (kernel_log_level < KERNEL_LOG_LEVEL_WARN) {
        return;
    }

    // Obtain the list of variable arguments
    va_list args;

    // Indicate this is an 'error' type of message
    printf("warn: ");

    // Pass the message variable arguments to vprintf
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");

}

/**
 * Prints a kernel log message to the host with an info log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_info(char *msg, ...) {
    // Return if our log level is less than info
    if (kernel_log_level < KERNEL_LOG_LEVEL_INFO) {
        return;
    }

    // Obtain the list of variable arguments
    va_list args;

    // Indicate this is an 'info' type of message
    printf("info: ");

    // Pass the message variable arguments to vprintf
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a debug log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_debug(char *msg, ...) {
    if (kernel_log_level < KERNEL_LOG_LEVEL_DEBUG) {
        return;
    }

    // Obtain the list of variable arguments
    va_list args;

    // Indicate this is an 'error' type of message
    printf("debug: ");

    // Pass the message variable arguments to vprintf
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a trace log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_trace(char *msg, ...) {
    if (kernel_log_level < KERNEL_LOG_LEVEL_TRACE) {
        return;
    }

    // Obtain the list of variable arguments
    va_list args;

    // Indicate this is an 'error' type of message
    printf("trace: ");

    // Pass the message variable arguments to vprintf
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Triggers a kernel panic that does the following:
 *   - Displays a panic message on the host console
 *   - Triggers a breakpiont (if running through GDB)
 *   - aborts/exits the operating system program
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_panic(char *msg, ...) {
    va_list args;

    printf("panic: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");

    breakpoint();
    exit(1);
}

/**
 * Set the log level to the value specified and return the log level.
 */
int kernel_set_log_level(int log_level) {
    if(log_level < KERNEL_LOG_LEVEL_NONE || log_level > KERNEL_LOG_LEVEL_ALL) {
        kernel_log_error("Invalid log level. Log level not changed.");
    }
    else {
        switch(log_level) {
            case 0:
                printf("Kernel Log Level Set: NONE\n");
                break;
            case 1:
                printf("Kernel Log Level Set: ERROR\n");
                break;
            case 2:
                printf("Kernel Log Level Set: WARN\n");
                break;
            case 3:
                printf("Kernel Log Level Set: INFO\n");
                break;
            case 4:
                printf("Kernel Log Level Set: DEBUG\n");
                break;
            case 5:
                printf("Kernel Log Level Set: TRACE\n");
                break;
            case 6:
                printf("Kernel Log Level Set: ALL\n");
                break;
        }
        kernel_log_level = log_level;
    }
    return kernel_log_level;
}

/**
 * Returns the current log level.
 */
int kernel_get_log_level() {
    return kernel_log_level;
}

/**
 * Prints an exit message to the debug console and the VGA display,
 * then exits the program.
 */
void kernel_exit() {
    printf("Exiting %s\n", OS_NAME);

    vga_clear();
    vga_set_bg(VGA_COLOR_RED);
    vga_set_fg(VGA_COLOR_LIGHT_GREY);

    vga_set_xy(0, 0);
    vga_printf("%*s", 80, "");

    int exit_message_xposition = (VGA_WIDTH / 2) - 6;
    vga_set_xy(exit_message_xposition, 0);
    vga_printf("Exiting %s\n", OS_NAME);
    exit(0);
}

void kernel_context_enter(trapframe_t *trapframe) {
    // Save currently running trapframe.
    if(active_proc) {
        active_proc->trapframe = trapframe;
    }

    // Process interrupt that occured.
    interrupts_irq_handler(trapframe->interrupt);

    // Run the Scheduler.
    scheduler_run();

    if(!active_proc) {
        kernel_panic("No active process!");
    }

    // Exit kernel context.
    kernel_context_exit(active_proc->trapframe);
}
