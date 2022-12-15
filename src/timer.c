/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Timer Implementation
 */
#include <spede/string.h>

#include "interrupts.h"
#include "kernel.h"
#include "queue.h"
#include "timer.h"

/**
 * Data structures
 */
// Timer data structure
typedef struct timer_t {
    void (*callback)();    // Function to call when the interval occurs
    int interval;          // Interval in which the timer will be called
    int repeat;            // Indicate how many intervals to repeat (-1 repeats forever)
} timer_t;

/**
 * Variables
 */

// Number of timer ticks that have occured
int timer_ticks;

// Timers table; each item in the array is a timer_t struct
timer_t timers[TIMERS_MAX];

// Timer allocator; used to allocate indexes into the timers table
queue_t timer_allocator;


/**
 * Registers a new callback to be called at the specified interval
 * @param func_ptr - function pointer to be called
 * @param interval - number of ticks before the callback is performed
 * @param repeat   - Indicate how many intervals to repeat (-1 should repeat forever)
 *
 * @return the allocated timer id or -1 for errors
 */
int timer_callback_register(void (*func_ptr)(), int interval, int repeat) {
    int timer_id = -1;
    timer_t *timer;

    if (!func_ptr) {
        kernel_log_error("timer: invalid function pointer");
        return -1;
    }

    // Obtain a timer id
    if (queue_out(&timer_allocator, &timer_id) != 0) {
        kernel_log_error("timer: unable to allocate a timer");
        return -1;
    }

    timer = &timers[timer_id];

    // Set the callback function for the timer.
    timer->callback = func_ptr;

    // Set the interval value for the timer.
    timer->interval = interval;

    // Set the repeat value for the timer.
    timer->repeat = repeat;

    kernel_log_info("Timer callback registered timers[%d].", timer_id);
    return timer_id;
}

/**
 * Unregisters the specified callback
 * @param id
 *
 * @return 0 on success, -1 on error
 */
int timer_callback_unregister(int id) {
    timer_t *timer;

    if (id < 0 || id >= TIMERS_MAX) {
        kernel_log_error("timer: callback id out of range: %d", id);
        return -1;
    }

    timer = &timers[id];
    memset(timer, 0, sizeof(timer_t));

    if (queue_in(&timer_allocator, id) != 0) {
        kernel_log_error("timer: unable to queue timer entry back to allocator");
        return -1;
    }

    return 0;
}

/**
 * Returns the number of ticks that have occured since startup
 *
 * @return timer_ticks
 */
int timer_get_ticks() {
    return timer_ticks;
}

/**
 * Timer IRQ Handler
 *
 * Should perform the following:
 *   - Increment the timer ticks every time the timer occurs
 *   - Handle each registered timer
 *     - If the interval is hit, run the callback function
 *     - Handle timer repeats
 */
void timer_irq_handler(void) {
    // Increment the timer_ticks value
    timer_ticks++;

    // Iterate through the timers table
    for(int i = 0; i < TIMERS_MAX; i++) {

        // If we have a valid callback, check if it needs to be called
        if(timers[i].callback) {

            // If the timer interval is hit, run the callback function
            if(timer_ticks % timers[i].interval == 0) {
                (*timers[i].callback)();
            }

            // If the timer repeat is greater than 0, decrement
            if(timers[i].repeat > 0) {
                timers[i].repeat--;
            }
            // If the timer repeat is equal to 0, unregister the timer
            else if(timers[i].repeat == 0) {
                timer_callback_unregister(i);
            }
            // If the timer repeat is less than 0, do nothing
        }
    }
}

/**
 * Initializes timer related data structures and variables
 */
void timer_init(void) {
    kernel_log_info("Initializing timer");

    // Set the starting tick value
    timer_ticks = 0;

    // Initialize the timers data structures
    for(int i = 0; i < TIMERS_MAX; i++) {
        timers[i].callback          = NULL;
        timers[i].interval          = 0;
        timers[i].repeat            = 0;
    }

    // Initialize the timer callback allocator queue
    queue_init(&timer_allocator);

    // Populate items into the allocator queue
    for(int i = 0; i < TIMERS_MAX; i++) {
        if(queue_in(&timer_allocator, i) != 0) {
            kernel_log_warn("timer: Unable to queue timer allocator %d", i);
        }
    }

    // Register the Timer IRQ with the isr_entry_timer and timer_irq_handler
    interrupts_irq_register(IRQ_TIMER, isr_entry_timer, timer_irq_handler);
}
