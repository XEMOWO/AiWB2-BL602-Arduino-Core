/*
 * wiring_digital.c — pinMode / digitalWrite / digitalRead.
 *
 * Wraps the SDK's bl_gpio_* convenience API. Pin numbers are translated
 * through digital_pin_to_gpio[] from the board's pins_arduino.h.
 */
#include "Arduino.h"

#include <bl_gpio.h>

/* Current mode of every Arduino pin, so read/write can auto-configure. */
static uint8_t pin_modes[NUM_DIGITAL_PINS];

void pinMode(uint8_t pin, uint8_t mode)
{
    uint8_t gpio;

    if (pin >= NUM_DIGITAL_PINS) {
        return;
    }
    gpio = digital_pin_to_gpio[pin];

    switch (mode) {
    case OUTPUT:
        bl_gpio_enable_output(gpio, 0, 0);
        break;
    case INPUT_PULLUP:
        bl_gpio_enable_input(gpio, 1, 0);
        break;
    case INPUT_PULLDOWN:
        bl_gpio_enable_input(gpio, 0, 1);
        break;
    case INPUT:
    default:
        bl_gpio_enable_input(gpio, 0, 0);
        break;
    }
    pin_modes[pin] = mode;
}

void digitalWrite(uint8_t pin, uint8_t value)
{
    uint8_t gpio;

    if (pin >= NUM_DIGITAL_PINS) {
        return;
    }
    gpio = digital_pin_to_gpio[pin];

    /* Auto-configure as output if pinMode() was never called, like the
     * AVR/ESP cores do. */
    if (pin_modes[pin] != OUTPUT) {
        bl_gpio_enable_output(gpio, 0, 0);
        pin_modes[pin] = OUTPUT;
    }
    bl_gpio_output_set(gpio, value ? 1 : 0);
}

int digitalRead(uint8_t pin)
{
    uint8_t gpio;

    if (pin >= NUM_DIGITAL_PINS) {
        return LOW;
    }
    gpio = digital_pin_to_gpio[pin];

    if (pin_modes[pin] != INPUT && pin_modes[pin] != INPUT_PULLUP &&
        pin_modes[pin] != INPUT_PULLDOWN) {
        bl_gpio_enable_input(gpio, 0, 0);
        pin_modes[pin] = INPUT;
    }
    return bl_gpio_input_get_value(gpio) ? HIGH : LOW;
}
