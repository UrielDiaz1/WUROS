#include <spede/machine/io.h>
#include <spede/stdarg.h>
#include <spede/stdio.h>

#include "kernel.h"
#include "vga.h"

// VGA Address Port -> Set the register address to write data into
#define VGA_PORT_ADDR 0x3D4
// VGA Data Port -> The data to be written into the register
#define VGA_PORT_DATA 0x3D5

// Function Declarations.
void scroll_up(void);
void vga_scrolling_enable(void);
void vga_scrolling_disable(void);

// VGA Buffer. Write to this buffer to print to output.
unsigned short *vga_buf = VGA_BASE;

// Current x position (column)
int vga_pos_x;

// Current y position (row)
int vga_pos_y;

// Current background color, default to black
int vga_color_bg;

// Current foreground color, default to light grey
int vga_color_fg;

// VGA text mode cursor status
int vga_cursor;

// VGA scrolling status
int scrolling_enabled;

/**
 * Initializes the VGA driver and configuration
 *  - Defaults variables
 *  - Clears the screen
 */
void vga_init(void) {
    kernel_log_info("Initializing VGA driver");

    vga_pos_x  = 0;
    vga_pos_y  = 0;
    vga_color_bg = VGA_COLOR_BLACK;
    vga_color_fg = VGA_COLOR_LIGHT_GREY;
    scrolling_enabled = 0;

    // Disable the cursor and scrolling by default.
    vga_cursor_disable();
    vga_scrolling_disable();

    // Clear the screen
    vga_clear();
}

/**
 * Sets the cursor position to the current VGA row/column (x/y)
 * position if the cursor is enabled.
 */
void vga_cursor_update(void) {
    if (vga_cursor) {
        unsigned short pos = vga_pos_x + vga_pos_y * VGA_WIDTH;

        outportb(VGA_PORT_ADDR, 0x0F);
        outportb(VGA_PORT_DATA, (unsigned char) (pos & 0xFF));

        outportb(VGA_PORT_ADDR, 0x0E);
        outportb(VGA_PORT_DATA, (unsigned char) ((pos >> 8) & 0xFF));
    }
}

/**
 * Enables the VGA text mode cursor
 */
void vga_cursor_enable(void) {
    vga_cursor = 0;

    // The cursor will be drawn between the scanlines defined
    // in the "Cursor Start Register" (0x0A) and the
    // "Cursor End Register" (0x0B)

    // To ensure that we do not change bits we are not intending,
    // read the current register state and mask off the bits we
    // want to save

    // Set the cursor starting scanline
    outportb(VGA_PORT_ADDR, 0x0A);
    outportb(VGA_PORT_DATA, (inportb(VGA_PORT_DATA) & 0xC0) | 0xE);

    // Set the cursor ending scanline
    // Ensure that bit 5 is not set so the cursor will be enabled
    outportb(VGA_PORT_ADDR, 0x0B);
    outportb(VGA_PORT_DATA, (inportb(VGA_PORT_DATA) & 0xE0) | 0xF);
}

/**
 * Disables the VGA text mode cursor
 */
void vga_cursor_disable(void) {
    vga_cursor = 0;

    // The cursor can be disabled by setting
    // bit 5 in the "Cursor Start Register" (0xA)
    outportb(VGA_PORT_ADDR, 0x0A);
    outportb(VGA_PORT_DATA, 0x20);
}

/**
 * Enables vga display scrolling.
 */
void vga_scrolling_enable(void) {
    scrolling_enabled = 1;
}

/*
 * Disables vga display scrolling.
 */
void vga_scrolling_disable(void) {
    scrolling_enabled = 0;
}

/**
 * Clears the VGA output and sets the background and foreground colors
 */
void vga_clear(void) {
    unsigned short *vga_buf = VGA_BASE;

    for (unsigned int i = 0; i < (VGA_WIDTH * VGA_HEIGHT); i++) {
        vga_buf[i] = VGA_CHAR(vga_color_bg, vga_color_fg, 0x00);
    }

    vga_set_xy(0, 0);
}

/**
 * Sets the current X/Y (column/row) position
 *
 * @param x - x position (0 to VGA_WIDTH-1)
 * @param y - y position (0 to VGA_HEIGHT-1)
 * @notes If the input parameters exceed the valid range, the position
 *        will be set to the range boundary (min or max)
 */
void vga_set_xy(int x, int y) {
    // Set equal to x if within width range.
    if(x >= 0 || x <= VGA_WIDTH-1) {
        vga_pos_x = x;
    }
    // Data validition to ensure x-position remains within range.
    else if (x < 0) {
        kernel_log_warn("Unexpected negative x-positions corrected.");
        vga_pos_x = 0;
    }
    else {
        vga_pos_x = VGA_WIDTH-1;
    }

    // Set equal to y if within height range.
    if(y >= 0 || y <= VGA_HEIGHT-1) {
        vga_pos_y = y;
    }
    // Data validation to ensure y-position remains within range.
    else if (y < 0) {
        kernel_log_warn("Unexpected negative y-positions corrected.");
        vga_pos_y = 0;
    }
    else {
        vga_pos_y = VGA_HEIGHT-1;
    }
    vga_cursor_update();
}

/**
 * Gets the current X (column) position
 * @return integer value of the column (between 0 and VGA_WIDTH-1)
 */
int vga_get_x(void) {
    return vga_pos_x;
}

/**
 * Gets the current Y (row) position
 * @return integer value of the row (between 0 and VGA_HEIGHT-1)
 */
int vga_get_y(void) {
    return vga_pos_y;
}

/**
 * Sets the background color.
 *
 * Does not modify any existing background colors, only sets it for
 * new operations.
 *
 * @param bg - background color
 */
void vga_set_bg(int bg) {
    // The acceptable bg color range is between 0x0 to 0x7.
    if(bg >= VGA_COLOR_BLACK && bg <= VGA_COLOR_LIGHT_GREY) {
        vga_color_bg = bg;
    }
    else {
        kernel_log_warn("Invalid background color. Bg remains unchanged.");
    }
}

/**
 * Gets the current background color
 * @return background color value
 */
int vga_get_bg(void) {
    return vga_color_bg;
}

/**
 * Sets the foreground/text color.
 *
 * Does not modify any existing foreground colors, only sets it for
 * new operations.
 *
 * @param color - foreground color
 */
void vga_set_fg(int fg) {
    // The acceptable fg color range is between 0x0 to 0xF.
    if(fg >= VGA_COLOR_BLACK && fg <= VGA_COLOR_WHITE) {
        vga_color_fg = fg;
    }
    else {
        kernel_log_warn("Invalid foreground color. Fg remains unchanged.");
    }
}

/**
 * Gets the current foreground color
 * @return foreground color value
 */
int vga_get_fg(void) {
    return vga_color_fg;
}

/**
 * Prints the character on the screen.
 *
 * Does not change the x/y position, simply sets the character
 * at the current x/y position using existing background and foreground
 * colors. Does not change the cursor position.
 *
 * @param c - Character to print
 */
void vga_setc(char c) {
    // (VGA_WIDTH * vga_pos_y) determines the row. Ex: 80 * 6 = 480, which is the start
    // of the 6th row. Adding vga_pos_x updates the column.
    vga_buf[(VGA_WIDTH * vga_pos_y) + vga_pos_x] = VGA_CHAR(vga_color_bg, vga_color_fg, c);
    vga_cursor_update();
}

/**
 * Prints a character on the screen.
 *
 * When a character is printed, will do the following:
 *  - Update the x and y positions
 *  - If needed, will wrap from the end of the current line to the
 *    start of the next line
 *  - If the last line is reached, will ensure that all text is
 *    scrolled up
 *  - Special characters are handled as such:
 *    - tab character (\t) prints 'tab_stop' spaces
 *    - backspace (\b) character moves the character back one position,
 *      and prints a NULL character at that location.
 *
 * @param c - character to print
 */
void vga_putc(char c) {
    // Tab character handling (equivalent to 4 spaces).
    if(c == '\t') {
        // If tab is used near end of line, do end of line instead.
        if(vga_get_x() >= 75) {
            vga_putc('\n');
        }
        else {
            for(int i = 0; i < 4; i++) {
                vga_putc(' ');
            }
        }

        vga_cursor_update();

        // The actual tab character does not need to be printed. Return early.
        return;
    }

    // Backspace handling.
    if(c == '\b') {
        // Do nothing if cursor at (0, 0).
        if(vga_pos_y == 0 && vga_pos_x == 0) {
            return;
        }

        // Moves buffer one character back.
        vga_buf[((VGA_WIDTH * vga_pos_y) + vga_pos_x) - 1] = vga_buf[VGA_WIDTH * vga_pos_y + vga_pos_x];

        // Clears the previous position.
        vga_buf[(VGA_WIDTH * vga_pos_y) + vga_pos_x] = VGA_CHAR(vga_color_bg, vga_color_fg, 0x00);

        // Updates x and y positions.
        // If positioned at the start of a column, wrap back to the end of the previous line.
        if(vga_pos_x == 0) {
            vga_pos_y--;
            vga_pos_x = VGA_WIDTH - 1;
        }
        else {
            vga_pos_x--;
        }
        vga_cursor_update();
        return;
    }

    // Carriage return handling.
    // Moves x-position to 0.
    if(c == '\r') {
        vga_pos_x = 0;

        vga_cursor_update();
        // Return early since nothing needs to be printed.
        return;
    }

    // New Line/Line Feed handling.
    // Increments the y position and sets the x position to 0.
    if(c == '\n') {
        if(vga_pos_y == (VGA_HEIGHT - 1)) {
            scroll_up();
        }
        else {
            vga_pos_y++;
        }
        vga_pos_x = 0;

        vga_cursor_update();

        // Since there's nothing to print, return early.
        return;
    }

    // Writes Character to buffer.
    vga_buf[(VGA_WIDTH * vga_pos_y) + vga_pos_x] = VGA_CHAR(vga_color_bg, vga_color_fg, c);

    // Update column.
    vga_pos_x++;

    // Wrap from the end of the current line to the start of the next line if necessary.
    if(vga_pos_x == (VGA_WIDTH)) {
        vga_pos_x = 0;

        // Update row.
        if(vga_pos_y == VGA_HEIGHT - 1) {
            scroll_up();
        }
        else {
            vga_pos_y++;
        }
    }
    vga_cursor_update();
}

/**
 * Prints a string on the screen.
 *
 * @param s - string to print
 */
void vga_puts(char *s) {
    int i = 0;
    while(s[i] != '\0') {
        vga_putc(s[i]);
        i++;
    }
}

/**
 * Prints a character on the screen at the specified x/y position and
 * with the specified background/foreground colors
 *
 * Does not change the "current" x/y position
 * Does not change the "current" background/foreground colors
 *
 * @param x - x position (0 to VGA_WIDTH-1)
 * @param y - y position (0 to VGA_HEIGHT-1)
 * @param bg - background color
 * @param fg - foreground color
 * @param c - character to print
 */
void vga_putc_at(int x, int y, int bg, int fg, char c) {
    vga_buf[(VGA_WIDTH * y) + x] = VGA_CHAR(bg, fg, c);
    vga_cursor_update();
}

/**
 * Prints a string on the screen at the specified x/y position and
 * with the specified background/foreground colors
 *
 * Does not change the "current" x/y position or background/foreground colors
 *
 * @param x - x position (0 to VGA_WIDTH-1)
 * @param y - y position (0 to VGA_HEIGHT-1)
 * @param bg - background color
 * @param fg - foreground color
 * @param s - string to print
 */
void vga_puts_at(int x, int y, int bg, int fg, char *s) {
    int i = 0;
    while(s[i] != '\0') {
        vga_putc_at(x, y, bg, fg, s[i]);

        // Updates column.
        x++;
        if(x >= 80) {
            x = 0;
            y++;
        }
        // Updates row.
        if(y >= 25) {
            scroll_up();
            y--;
        }
        i++;
    }
}

/**
* Scrolls up by removing first line and copying each following line up.
*/
void scroll_up(void) {
    if(scrolling_enabled == 1) {
        for(int i = 0; i < VGA_WIDTH; i++) {
            vga_buf[i] = VGA_CHAR(vga_color_bg, vga_color_fg, 0x00);
        }

        for(int i = VGA_WIDTH; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            vga_buf[i - VGA_WIDTH] = vga_buf[i];
            vga_buf[i] = VGA_CHAR(vga_color_bg, vga_color_fg, 0x00);
        }
    }
    else {
        kernel_log_warn("Unable to scroll display; scrolling is disabled.");
    }
}
