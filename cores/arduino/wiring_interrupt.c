/*
 * wiring_interrupt.c — attachInterrupt / detachInterrupt.
 *
 * BL602 has a single GPIO interrupt vector (GPIO_INT0_IRQn); all pins share
 * it. The HOSAL layer keeps a linked list of registered pins and dispatches
 * the shared vector (hosal_gpio_irq_set() registers the entry + enables the
 * IRQ, re-attaching the same pin replaces its handler).
 *
 * CHANGE has no hardware two-edge mode, so it is emulated: the ISR re-arms
 * the pin for the opposite edge before calling the user handler. A very fast
 * toggle can still be missed — acceptable for typical button/bounce use.
 *
 * Handlers run in CLIC interrupt context: keep them short, no delay(), no
 * blocking Serial prints — set a volatile flag and service it in loop().
 *
 * This is a C file, so the guard-less SDK headers are safe to include.
 */
#include "Arduino.h"

#include <hosal_gpio.h>
#include <bl_gpio.h>
#include <bl602_glb.h>

/* One entry per Arduino pin; the HOSAL ctx stores &irq_entries[pin] as its
 * user arg, so the shared ISR knows which pin fired. */
typedef struct {
    void (*fn)(void);
    uint8_t gpio;
    uint8_t mode;
} irq_entry_t;

static irq_entry_t irq_entries[NUM_DIGITAL_PINS];

/* Shared ISR dispatch: re-arm CHANGE for the opposite edge, then run the
 * user handler (which must stay short and non-blocking). */
static void arduino_isr_dispatch(void *arg)
{
    irq_entry_t *e = (irq_entry_t *)arg;

    if (e->mode == CHANGE) {
        uint8_t lvl = bl_gpio_input_get_value(e->gpio);
        bl_set_gpio_intmod(e->gpio, GLB_GPIO_INT_CONTROL_ASYNC,
                           lvl ? GLB_GPIO_INT_TRIG_NEG_PULSE
                               : GLB_GPIO_INT_TRIG_POS_PULSE);
    }
    if (e->fn) {
        e->fn();
    }
}

void attachInterrupt(uint8_t pin, void (*userFunc)(void), int mode)
{
    hosal_gpio_dev_t g;
    hosal_gpio_irq_trigger_t trig;

    if (pin >= NUM_DIGITAL_PINS || userFunc == NULL) {
        return;
    }

    switch (mode) {
    case RISING:
        trig = HOSAL_IRQ_TRIG_POS_PULSE;
        break;
    case FALLING:
        trig = HOSAL_IRQ_TRIG_NEG_PULSE;
        break;
    case LOW:
        trig = HOSAL_IRQ_TRIG_NEG_LEVEL;
        break;
    case HIGH:
        trig = HOSAL_IRQ_TRIG_POS_LEVEL;
        break;
    case CHANGE:
        trig = HOSAL_IRQ_TRIG_POS_PULSE; /* flipped to NEG in the ISR */
        break;
    default:
        return;
    }

    pinMode(pin, INPUT);
    irq_entries[pin].fn = userFunc;
    irq_entries[pin].gpio = digital_pin_to_gpio[pin];
    irq_entries[pin].mode = (uint8_t)mode;

    memset(&g, 0, sizeof g);
    g.port = irq_entries[pin].gpio; /* HOSAL uses port as the pin number */
    hosal_gpio_irq_set(&g, trig, arduino_isr_dispatch, &irq_entries[pin]);
}

void detachInterrupt(uint8_t pin)
{
    hosal_gpio_dev_t g;

    if (pin >= NUM_DIGITAL_PINS) {
        return;
    }
    irq_entries[pin].fn = NULL;

    memset(&g, 0, sizeof g);
    g.port = digital_pin_to_gpio[pin];
    hosal_gpio_irq_mask(&g, 1); /* mask = the pin never fires again */
}
