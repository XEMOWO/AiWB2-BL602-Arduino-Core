/*
 * variant.cpp — Ai-WB2-12F (BL602) board definition.
 *
 * Single definition of the pin tables declared in pins_arduino.h.
 * (Kept as a separate translation unit so the tables don't produce
 * "defined but not used" warnings under -Werror=all in files that
 * only include the header.)
 */
#include "Arduino.h"

const uint8_t digital_pin_to_gpio[NUM_DIGITAL_PINS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21
};

/* Board-level ADC inputs (only these GPIOs are broken out and ADC-capable):
 * A0=12(ch0) A1=4(ch1) A2=14(ch2) A3=5(ch4) A4=11(ch10) — SDK channel order. */
const uint8_t analog_pin_to_gpio[NUM_ANALOG_INPUTS] = {
    12, 4, 14, 5, 11
};
