/**
 * CPE/CSC 159 Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Bit Utilities
 */
#include "bit_util.h"

/**
 * Counts the number of bits that are set
 * @param value - the integer value to count bits in
 * @return number of bits that are set
 */
int bit_count(int value) {
    if(value <= 0) {
        return 0;
    }

    int count = 0;
    while(value) {
        // Binary AND operator (&). Ex: 0001 & 1001 = 0001.
        count += value & 1;

        // Logical Right Shift operator (>>). Ex: 0010 -> 0001.
        value >>= 1;
    }
    return count;
}

/**
 * Checks if the given bit is set
 * @param value - the integer value to test
 * @param bit - which bit to check
 * @return 1 if set, 0 if not set
 */
int bit_test(int value, int bit) {
    // Move the bit of interest to the first bit position.
    value >>= bit - 1;

    // Returns only 1 if the bit of interest is set.
    return (value & 1);
}

/**
 * Sets the specified bit in the given integer value
 * @param value - the integer value to modify
 * @param bit - which bit to set
 */
int bit_set(int value, int bit) {
    int set_bit = 1;

    // Bitwise Left Shift (<<). Ex: 0001 -> 0010.
    set_bit <<= bit - 1;

    // Sets the bit of interest by using Bitwise OR operator (|).
    // Note: XOR cannot be used because it could clear an already
    // set bit.
    return (value | set_bit);
}

/**
 * Clears the specified bit in the given integer value
 * @param value - the integer value to modify
 * @param bit - which bit to clear
 */
int bit_clear(int value, int bit) {
    int clear_bit = 1;

    clear_bit <<= bit - 1;
    // Binary 1's Complement Operator (~) flips all bits.
    // Ex: 0100 -> 1011.
    clear_bit = ~clear_bit;

    // Since our bit of interest is 0 in clear_bit, the AND operator
    // will ensure the result's bit of interst is also 0.
    return (value & clear_bit);
}

/**
 * Toggles the specified bit in the given integer value
 * @param value - the integer value to modify
 * @param bit - which bit to toggle
 */
int bit_toggle(int value, int bit) {
    int toggle_bit = 1;
    toggle_bit <<= bit - 1;

    // Bitwise XOR (^) ensures that only our bit of interest gets
    // flipped, while the other bits remain unchanged.
    return (value ^ toggle_bit);
}
