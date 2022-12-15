/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * TTY Definitions
 */

#include <spede/string.h>

#include "kernel.h"
#include "timer.h"
#include "tty.h"
#include "vga.h"
#include "syscall.h"
#include "syscall_common.h"
#include "ringbuf.h"

// Function Declarations.
void tty_refresh(void);

// TTY Table
struct tty_t tty_table[TTY_MAX];

// Current Active TTY
struct tty_t *active_tty;


/**
 * Returns the TTY structure for the given TTY number.
 * @param tty_number - tty number/identifier.
 * @return NULL on error or pointer to entry in the TTY table.
 */
struct tty_t *tty_get(int tty) {
    if(tty >= TTY_MAX || tty < 0) {
        kernel_log_error("Invalid TTY ID[%d].", tty);
        return NULL;
    }
    return &tty_table[tty];
}

/**
 * Write a character into the TTY process input buffer.
 * If the echo flag is set, will also write the character
 * into the TTY process output buffer.
 * @param c - character to write into input buffer.
 */
void tty_input(char c) {
    if(!active_tty) {
        kernel_log_debug("No active tty. Unable to write character into input buffer.");
    }
    // Writes into the input buffer.
    ringbuf_write(&active_tty->io_input, c);

    // Writes into output buffer if echo is set.
    if(active_tty->echo == 1) {
        ringbuf_write(&active_tty->io_output, c);
    }
}

/**
 * Returns ID of current active TTY.
 */
int tty_get_active() {
    if(!active_tty) {
        return -1;
    }
    return active_tty->id;
}

/**
 * Sets the active TTY to the selected TTY number
 * @param tty - TTY number
 */
void tty_select(int n) {
    if(n < 0 || n >= TTY_MAX) {
        kernel_log_error("Invalid TTY ID %d.", n);
    }

    if(!active_tty || active_tty->id != n) {
        active_tty = &tty_table[n];
        active_tty->refresh = 1;

        kernel_log_info("TTY [%d]: Selected.", n);
    }
}

/**
 * Refreshes the tty if needed
 */
void tty_refresh(void) {
    if (!active_tty) {
        kernel_panic("No TTY is selected!");
        return;
    }

    // Handles input through echo.
    char output_c = '\0';
    while(!ringbuf_is_empty(&active_tty->io_output)) {
        if(ringbuf_read(&active_tty->io_output, &output_c) == 0) {
            tty_update(output_c);
        }
    }
    ringbuf_flush(&active_tty->io_output);

    // Checks if there are new characters in the buffer that need to be written
    // on the screen.
    if(active_tty->refresh) {
        char c = '\0';
        int y = 0;
        int x = 0;

        for(int i = 0; i < TTY_BUF_SIZE; i++) {
            c = active_tty->buf[TTY_WIDTH * y + x];
            vga_putc_at(x, y, active_tty->color_bg, active_tty->color_fg, c);

            x++;
            // Wrap from the end of the current line to the start of the next line if necessary.
            if(x >= TTY_WIDTH) {
                x = 0;
                // Update row.
                if(y == TTY_HEIGHT - 1) {
                    // Do nothing.
                }
                else {
                    y++;
                }
            }
        }
    }
    // Reset the tty's refresh flag so we don't refresh unnecessarily
    active_tty->refresh = 0;
}

/**
 * Scrolls the TTY up one line into the scrollback buffer.
 * If the buffer is at the top, it will not scroll up further.
 */
void tty_scroll_up() {
    for(int i = 0; i < TTY_WIDTH; i++) {
        active_tty->buf[i] = VGA_CHAR(active_tty->color_bg, active_tty->color_fg, 0x00);
    }

    for(int i = TTY_WIDTH; i < TTY_WIDTH * TTY_HEIGHT; i++) {
        active_tty->buf[i - TTY_WIDTH] = active_tty->buf[i];
        active_tty->buf[i] = VGA_CHAR(active_tty->color_bg, active_tty->color_fg, 0x00);
    }
}

/**
 * Updates the active TTY with the given character.
 * @param c - character to update on the TTY screen output.
 */
void tty_update(char c) {
    if (!active_tty) {
        kernel_log_warn("No active TTY. Unable to update TTY.");
        return;
    }

    active_tty->refresh = 1;

    int originalPlacement = (TTY_WIDTH * active_tty->pos_y) + active_tty->pos_x;

    // Since this is a virtual wrapper around the VGA display, treat each
    // input character as you would for the VGA output
    //   Adjust the x/y positions as necessary
    //   Handle scrolling at the bottom

    // Instead of writing to the VGA display directly, you will write
    // to the tty buffer.
    //
    // If the screen should be updated, the refresh flag should be set
    // to trigger the the VGA update via the tty_refresh callback

    // Tab character handling (equivalent to 4 spaces).
    if(c == '\t') {
        // If tab is used near end of line, do end of line instead.
        if(active_tty->pos_x >= 75) {
            tty_update('\n');
        }
        else {
            for(int i = 0; i < 4; i++) {
                tty_update(' ');
            }
        }

        // vga_cursor_update();

        // The actual tab character does not need to be printed. Return early.
        return;
    }

    // Backspace handling.
    if(c == '\b') {
        // Do nothing if cursor at (0, 0).
        if(active_tty->pos_y == 0 && active_tty->pos_x == 0) {
            return;
        }

        // Moves buffer one character back.
        active_tty->buf[originalPlacement - 1] = active_tty->buf[originalPlacement];

        // Clears the previous position.
        active_tty->buf[originalPlacement] = VGA_CHAR(active_tty->color_bg, active_tty->color_fg, 0x00);

        // Updates x and y positions.
        // If positioned at the start of a column, wrap back to the end of the previous line.
        if(active_tty->pos_x == 0) {
            active_tty->pos_y--;
            active_tty->pos_x = TTY_WIDTH - 1;
        }
        else {
            active_tty->pos_x--;
        }
        // vga_cursor_update();
        return;
    }

    // Carriage return handling.
    // Moves x-position to 0.
    if(c == '\r') {
        active_tty->pos_x = 0;

        // vga_cursor_update();
        // Return early since nothing needs to be printed.
        return;
    }

    // New Line/Line Feed handling.
    // Increments the y position and sets the x position to 0.
    if(c == '\n') {
        if(active_tty->pos_y == (TTY_HEIGHT - 1)) {
            tty_scroll_up();
        }
        else {
            active_tty->pos_y++;
        }
        active_tty->pos_x = 0;

        // vga_cursor_update();

        // Since there's nothing to print, return early.
        return;
    }

    // Writes Character to buffer.
    active_tty->buf[originalPlacement] = VGA_CHAR(active_tty->color_bg, active_tty->color_fg, c);

    // Update column.
    active_tty->pos_x++;

    // Wrap from the end of the current line to the start of the next line if necessary.
    if(active_tty->pos_x == (TTY_WIDTH)) {
        active_tty->pos_x = 0;

        // Update row.
        if(active_tty->pos_y == TTY_HEIGHT - 1) {
            tty_scroll_up();
        }
        else {
            active_tty->pos_y++;
        }
    }
    kernel_log_debug("Wrote %c character into tty buffer.", active_tty->buf[originalPlacement]);
    // vga_cursor_update();
}

/**
 * Initializes all TTY data structures and memory
 * Selects TTY 0 to be the default
 */
void tty_init(void) {
    kernel_log_info("tty: Initializing TTY driver");

    // Initialize the tty_table
    for(int i = 0; i < TTY_MAX; i++) {
        tty_table[i].id = i;

        for(int j = 0; j < TTY_BUF_SIZE; j++) {
            tty_table[i].buf[j] = '\0';
        }
        tty_table[i].refresh = 0;
        tty_table[i].echo    = 0;

        tty_table[i].color_bg = VGA_COLOR_BLACK;
        tty_table[i].color_fg = VGA_COLOR_LIGHT_GREY;

        tty_table[i].pos_x      = 0;
        tty_table[i].pos_y      = 0;
        tty_table[i].pos_scroll = 0;

        ringbuf_init(&tty_table[i].io_input);
        ringbuf_init(&tty_table[i].io_output);
    }

    // Select tty 0 to start with
    tty_select(0);

    // Register a timer callback to update the screen on a regular interval
    timer_callback_register(&tty_refresh, 1, -1);
}
