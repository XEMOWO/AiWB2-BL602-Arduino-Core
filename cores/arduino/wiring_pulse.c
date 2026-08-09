/*
 * wiring_pulse.c — pulseIn().
 *
 * Measures the width (in microseconds) of a pulse on a pin: waits for the
 * opposite level, waits for the target level, then times the pulse. Polls
 * the GPIO in a busy loop; the pin must already be configured as input
 * (pinMode(pin, INPUT) or INPUT_PULLUP).
 *
 * Returns 0 on timeout or if the pulse never completes within timeout us.
 */
#include "Arduino.h"

#include <bl_gpio.h>

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout)
{
    uint8_t gpio;
    unsigned long start, t0;

    if (pin >= NUM_DIGITAL_PINS) {
        return 0;
    }
    gpio = digital_pin_to_gpio[pin];

    /* Wait for the idle (opposite) level first, so a pulse already in
     * progress is measured from its start. */
    start = micros();
    while (bl_gpio_input_get_value(gpio) == state) {
        if ((unsigned long)(micros() - start) > timeout) {
            return 0;
        }
    }
    /* Wait for the leading edge of the target level. */
    while (bl_gpio_input_get_value(gpio) != state) {
        if ((unsigned long)(micros() - start) > timeout) {
            return 0;
        }
    }
    /* Time the pulse itself. */
    t0 = micros();
    while (bl_gpio_input_get_value(gpio) == state) {
        if ((unsigned long)(micros() - t0) > timeout) {
            return 0;
        }
    }
    return (unsigned long)(micros() - t0);
}
