#include <spede/stdio.h>
#include <spede/machine/io.h>
#include <spede/stdbool.h>      // for bool type

#include "kernel.h"
#include "keyboard.h"
#include "bit_util.h"
#include "tty.h"
#include "interrupts.h"
#include "kproc.h"

// Keyboard data port
#define KBD_PORT_DATA           0x60

// Keyboard status port
#define KBD_PORT_STAT           0x64

// Keyboard scancode definitions
#define KEY_CTRL_L              0x1D
#define KEY_CTRL_R              0xE01D

#define KEY_ALT_L               0x38
#define KEY_ALT_R               0xE038

#define KEY_SHIFT_L             0x2A
#define KEY_SHIFT_R             0x36

#define KEY_CAPS                0x3A
#define KEY_NUMLOCK             0x45
#define KEY_SCROLLLOCK          0x46

// Function declarations.
unsigned int shift_decode(unsigned int soff_ascii, unsigned int son_ascii);
unsigned int letter_decode(unsigned int lowercase, unsigned int uppercase);
unsigned int numlock_decode(unsigned int numpad, unsigned int func_nav);
void keyboard_irq_handler(void);

// Special Key States.
bool CAPS_LOCK_ON;
bool SHIFT_L_ON;
bool SHIFT_R_ON;
bool NUMLOCK_ON;
bool CTRL_L_ON;
bool CTRL_R_ON;
bool ALT_L_ON;
bool ALT_R_ON;
int ESC_COUNTER;

/**
 * Initializes keyboard data structures and variables
 */
void keyboard_init() {
    kernel_log_info("Initializing keyboard");

    CAPS_LOCK_ON   = false;
    SHIFT_L_ON     = false;
    SHIFT_R_ON     = false;
    NUMLOCK_ON     = false;
    CTRL_L_ON      = false;
    CTRL_R_ON      = false;
    ALT_L_ON       = false;
    ALT_R_ON       = false;
    ESC_COUNTER    = 0;

    interrupts_irq_register(IRQ_KEYBOARD, isr_entry_keyboard, keyboard_irq_handler);
}

/**
 * Scans for keyboard input and returns the raw character data
 * @return raw character data from the keyboard
 */
unsigned int keyboard_scan(void) {
    // Reads and returns the raw data from the keyboard.
    return inportb(KBD_PORT_DATA);
}

/**
 * Polls for a keyboard character to be entered.
 *
 * If a keyboard character data is present, will scan and return
 * the decoded keyboard output.
 *
 * @return decoded character or KEY_NULL (0) for any character
 *         that cannot be decoded
 */
unsigned int keyboard_poll(void) {
    unsigned int c = KEY_NULL;

    // Check status of keyboard to determine if data is available.
    char status = inportb(KBD_PORT_STAT);

    // The first bit of status indicates whether keyboard data is available.
    // If the first bit is set, scan the keyboard data, then decode it.
    // BIts are numbered 1 through 8.
    if(bit_test(status, 1)) {
        c = keyboard_scan();
        c = keyboard_decode(c);
    }

    return c;
}

/**
 * Blocks until a keyboard character has been entered
 * @return decoded character entered by the keyboard or KEY_NULL
 *         for any character that cannot be decoded
 */
unsigned int keyboard_getc(void) {
    unsigned int c = KEY_NULL;
    while ((c = keyboard_poll()) == KEY_NULL);
    return c;
}

/**
 * Processes raw keyboard input and decodes it.
 *
 * Should keep track of the keyboard status for the following keys:
 *   SHIFT, CTRL, ALT, CAPS, NUMLOCK
 *
 * For all other characters, they should be decoded/mapped to ASCII
 * or ASCII-friendly characters.
 *
 * For any character that cannot be mapped, KEY_NULL should be returned.
 */
unsigned int keyboard_decode(unsigned int c) {
    // Holds decoded data.
    unsigned int d = KEY_NULL;

    //test proc to create
    void *test_proc = kproc_test;

    // Decodes the raw keyboard input into the appropriate ASCII codes.
    switch(c) {
        case 0x00:
            kernel_log_error("Keyboard scan code 0x00.");
            break;
        case 0x01:  // Esc
            d = 27;
            break;
        case 0x02:  // 1 OR !
            d = shift_decode(49, 33);
            break;
        case 0x03:  // 2 OR @
            d = shift_decode(50, 64);
            break;
        case 0x04:  // 3 OR #
            d = shift_decode(51, 35);
            break;
        case 0x05:  // 4 OR $
            d = shift_decode(52, 36);
            break;
        case 0x06:  // 5 OR %
            d = shift_decode(53, 37);
            break;
        case 0x07:  // 6 OR ^
            d = shift_decode(54, 94);
            break;
        case 0x08:  // 7 OR &
            d = shift_decode(55, 38);
            break;
        case 0x09:  // 8 OR *
            d = shift_decode(56, 42);
            break;
        case 0x0a:  // 9 OR (
            d = shift_decode(57, 40);
            break;
        case 0x0b:  // 0 OR )
            d = shift_decode(48, 41);
            break;
        case 0x0c:  // - OR _
            d = shift_decode(45, 95);
            break;
        case 0x0d:  // = OR +
            d = shift_decode(61, 43);
            break;
        case 0x0e:  // Backspace
            d = 8;
            break;
        case 0x0f:  // Tab
            d = 9;
            break;
        case 0x10:  // q OR Q
            d = letter_decode(113, 81);
            break;
        case 0x11:  // w OR W
            d = letter_decode(119, 87);
            break;
        case 0x12:  // e OR E
            d = letter_decode(101, 69);
            break;
        case 0x13:  // r OR R
            d = letter_decode(114, 82);
            break;
        case 0x14:  // t OR T
            d = letter_decode(116, 84);
            break;
        case 0x15:  // y OR Y
            d = letter_decode(121, 89);
            break;
        case 0x16:  // u OR U
            d = letter_decode(117, 85);
            break;
        case 0x17:  // i OR I
            d = letter_decode(105, 73);
            break;
        case 0x18:  // o OR O
            d = letter_decode(111, 79);
            break;
        case 0x19:  // p OR P
            d = letter_decode(112, 80);
            break;
        case 0x1a:  // [ OR {
            d = shift_decode(91, 123);
            break;
        case 0x1b:  // ] OR }
            d = shift_decode(93, 125);
            break;
        case 0x1c:  // Enter (Line Feed)
            d = 10;
            break;
        case KEY_CTRL_L:  // Left Ctrl
            CTRL_L_ON = true;
            break;
        case KEY_CTRL_L + KEY_RELEASE:
            CTRL_L_ON = false;
            break;
        case KEY_CTRL_R:  // Right Ctrl
            CTRL_R_ON = true;
            break;
        case KEY_CTRL_R + KEY_RELEASE:
            CTRL_R_ON = false;
            break;
        case 0x1e:  // a OR A
            d = letter_decode(97, 65);
            break;
        case 0x1f:  // s OR A
            d = letter_decode(115, 83);
            break;
        case 0x20:  // d OR D
            d = letter_decode(100, 68);
            break;
        case 0x21:  // f or F
            d = letter_decode(102, 70);
            break;
        case 0x22:  // g or G
            d = letter_decode(103, 71);
            break;
        case 0x23:  // h OR H
            d = letter_decode(104, 72);
            break;
        case 0x24:  // j or J
            d = letter_decode(106, 74);
            break;
        case 0x25:  // k or K
            d = letter_decode(107, 75);
            break;
        case 0x26:  // l or L
            d = letter_decode(108, 76);
            break;
        case 0x27:  // ; OR :
            d = shift_decode(59, 58);
            break;
        case 0x28:  // ' OR "
            d = shift_decode(39, 34);
            break;
        case 0x29:  // ` OR ~
            d = shift_decode(96, 126);
            break;
        case KEY_SHIFT_L:               // Left Shift Press
            SHIFT_L_ON = true;
            break;
        case KEY_SHIFT_L + KEY_RELEASE: // Left Shift Release
            SHIFT_L_ON = false;
            break;
        case 0x2b:  // \ OR |
            d = shift_decode(92, 124);
            break;
        case 0x2c:  // z OR Z
            d = letter_decode(122, 90);
            break;
        case 0x2d:  // x OR X
            d = letter_decode(120, 88);
            break;
        case 0x2e:  // c OR C
            d = letter_decode(99, 67);
            break;
        case 0x2f:  // c or C
            d = letter_decode(118, 86);
            break;
        case 0x30:  // b or B
            d = letter_decode(98, 66);
            break;
        case 0x31:  // n or N
            d = letter_decode(110, 78);
            break;
        case 0x32:  // m or M
            d = letter_decode(109, 77);
            break;
        case 0x33:  // , OR <
            d = shift_decode(44, 60);
            break;
        case 0x34:  // . OR >
            d = shift_decode(46, 62);
            break;
        case 0x35:  // / OR ?
            d = shift_decode(47, 63);
            break;
        case KEY_SHIFT_R:               // Right Shift Press
            SHIFT_R_ON = true;
            break;
        case KEY_SHIFT_R + KEY_RELEASE: // Right Shift Release
            SHIFT_R_ON = false;
            break;
        case 0x37:  //Numpad *
            d = 42;
            break;
        case KEY_ALT_L:                 // Left alt
            ALT_L_ON = true;
            break;
        case KEY_ALT_L + KEY_RELEASE:
            ALT_L_ON = false;
            break;
        case KEY_ALT_R:
            ALT_R_ON = true;
            break;
        case KEY_ALT_R + KEY_RELEASE:
            ALT_R_ON = false;
            break;
        case 0x39:  //Spacebar
            d = 32;
            break;
        case 0x3a:  // Caps Lock
            CAPS_LOCK_ON = !CAPS_LOCK_ON;
            break;
        case 0x3b:
            d = KEY_F1;
            break;
        case 0x3c:
            d = KEY_F2;
            break;
        case 0x3d:
            d = KEY_F3;
            break;
        case 0x3e:
            d = KEY_F4;;
            break;
        case 0x3f:
            d = KEY_F5;
            break;
        case 0x40:
            d = KEY_F6;
            break;
        case 0x41:
            d = KEY_F7;
            break;
        case 0x42:
            d = KEY_F8;
            break;
        case 0x43:
            d = KEY_F9;
            break;
        case 0x44:
            d = KEY_F10;
            break;
        case KEY_NUMLOCK:
            NUMLOCK_ON = !NUMLOCK_ON;
            break;
        case KEY_SCROLLLOCK:
            //TODO HANDLE BELOW
            break;
        case 0x47:  //Numpad 7 OR Home
            d = numlock_decode(55, KEY_HOME);
            break;
        case 0x48:  //Numpad 8 OR  Up Key
            d = numlock_decode(56, KEY_UP);
            break;
        case 0x49:  // Numpad 9 OR PgUp Key
            d = numlock_decode(57, KEY_PAGE_UP);
            break;
        case 0x4a:  //NumPad -
            d = 45;
            break;
        case 0x4b:  // Numpad 4 OR Left Key
            d = numlock_decode(52, KEY_LEFT);
            break;
        case 0x4c:  // Numpad 5
            d = numlock_decode(53, KEY_NULL);
            break;
        case 0x4d:  // Numpad 6 OR Right Key
            d = numlock_decode(54, KEY_RIGHT);
            break;
        case 0x4e:  // Numpad +
            d = 43;
            break;
        case 0x4f:  // Numpad 1 OR End Key
            d = numlock_decode(49, KEY_END);
            break;
        case 0x50:  // Numpad 2 OR Down Key
            d = numlock_decode(50, KEY_DOWN);
            break;
        case 0x51:  // Numpad 3 OR PgDn Key
            d = numlock_decode(51, KEY_PAGE_DOWN);
            break;
        case 0x52:  //Numpad 0 OR Insert Key
            d = numlock_decode(48, KEY_INSERT);
            break;
        case 0x53:  //Numpad . OR Delete Key
            d = numlock_decode(46, KEY_DELETE);
            break;
        case 0x57:
            d = KEY_F11;
            break;
        case 0x58:
            d = KEY_F12;
            break;
        default:
            d = KEY_NULL;
    }


    // Trigger kernel exit when the Escape key is triggered
    // 3 times consecutively.
    if(d == 27) {
        ESC_COUNTER++;
        kernel_log_trace("ESC Counter increased to %d/3.", ESC_COUNTER);
        if(ESC_COUNTER == 3) {
            kernel_exit();
        }
    }
    else if(c == 0x01 + KEY_RELEASE) {
        // Do nothing.
    }
    // Reset counter if a different key is triggered.
    else {
        ESC_COUNTER = 0;
    }

    // Reduce kernel log level when CTRL and '-' are pressed together.
    if(d == 45 && (CTRL_L_ON == true || CTRL_R_ON == true)) {
        kernel_set_log_level(kernel_get_log_level() - 1);
        return KEY_NULL;
    }

    // Increase kernel log level when CTRL and '+' are pressed together.
    if(d == 61 && (CTRL_L_ON == true || CTRL_R_ON == true)) {
        kernel_set_log_level(kernel_get_log_level() + 1);
        return KEY_NULL;
    }

    // When ALT + Number key are pressed, send pressed number to tty_select().
    if((d >= 48 && d <= 57) && (ALT_L_ON == true || ALT_R_ON == true)) {
        tty_select(d - 48);
        return KEY_NULL;
    }

    //When CTRL + n is pressed together, create a new process.
    if(d == 110 && (CTRL_L_ON == true || CTRL_R_ON == true)) {
        kproc_create(test_proc, "test", PROC_TYPE_USER);
        // Avoids printing characters when the intention is to create processes.
        return KEY_NULL;
    }

    //When CTRL + q is pressed together, create a new process.
    if(d == 113 && (CTRL_L_ON == true || CTRL_R_ON == true)) {
        kproc_destroy(active_proc);
        // Avoids printing characters when the intention is to destroy processes.
        return KEY_NULL;
    }

    return d;
}

/**
* Decode keyboard data based on Shift state.
* @param soff_ascii  ASCII Code for symbol with Shift Off.
* @param son_ascii   ASCII Code for symbol with Shift On.
* @return Appropriate ASCII code based on the shift state.
*/
unsigned int shift_decode(unsigned int soff_ascii, unsigned int son_ascii) {
    if(SHIFT_L_ON || SHIFT_R_ON) {
        return son_ascii;
    }
    else {
        return soff_ascii;
    }
}

/**
* Decode keyboard data based on CAPS LOCK and Shift state.
* @param uppercase ASCII Code for uppercase letter.
* @param lowercase ASCII Code for lowercase letter.
* @return Appropriate ASCII code based on the CAPS LOCK and Shift states.
*/
unsigned int letter_decode(unsigned int lowercase, unsigned int uppercase) {
    bool shift = false;

    // Simplifies shift state and aids readability.
    if(SHIFT_L_ON || SHIFT_R_ON) {
        shift = true;
    }

    // XOR operation. Uppercase only when either CAPS LOCK or Shift are
    // true, but not both or neither.
    if(CAPS_LOCK_ON == !shift) {
        return uppercase;
    }
    else {
        return lowercase;
    }
}
/**
* Decode keyboard data based on NUMLOCK state.
* @param numpad ASCII Codes for Numbers.
* @param numpad Codes for function and navigation.
* @return Appropriate code based on the NUMLOCK state.
*/
unsigned int numlock_decode(unsigned int numpad, unsigned int func_nav) {
    if(NUMLOCK_ON){
        return numpad;
    }
    else {
        return func_nav;
    }
}

/**
 * Timer IRQ Handler
 *
 * Should perform the following:
 *   - Call keyboard_poll to obtain a decoded keyboard character.
 *   - Print the character to the screen using tty_update.
 */
void keyboard_irq_handler(void) {
    char c = keyboard_poll();

    if(c) {
        tty_input(c);
    }
}
