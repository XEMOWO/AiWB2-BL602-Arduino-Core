/*
 * wiring_tone.c — tone() / noTone().
 *
 * Bit-bangs a square wave from a dedicated FreeRTOS task: the task toggles
 * the pin every half-period using delayMicroseconds(). This pins the CPU
 * while a tone plays and starves lower-priority tasks (the loop task runs at
 * priority 15, this task at 20) — keep durations short in demos.
 *
 * A single tone task serves one active pin; calling tone() again changes the
 * frequency/pin, noTone() stops it and drives the pin low.
 */
#include <FreeRTOS.h>
#include <task.h>

#include "Arduino.h"

#define TONE_TASK_PRIORITY 20 /* above the app/loop task */

static TaskHandle_t tone_task = NULL;
static volatile uint8_t  tone_pin = 0xFF;      /* 0xFF = no tone */
static volatile uint32_t tone_half_us = 0;     /* half period, 0 = idle */
static volatile uint32_t tone_cycles_left = 0; /* 0 = run forever */

static void tone_task_fn(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t h;
        uint8_t p;

        h = tone_half_us;
        p = tone_pin;
        if (h == 0 || p >= NUM_DIGITAL_PINS) {
            vTaskDelay(1);
            continue;
        }

        digitalWrite(p, HIGH);
        delayMicroseconds((unsigned int)h);
        digitalWrite(p, LOW);
        delayMicroseconds((unsigned int)h);

        if (tone_cycles_left && --tone_cycles_left == 0) {
            noTone(p);
        }
    }
}

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
    if (frequency == 0 || pin >= NUM_DIGITAL_PINS) {
        return;
    }
    pinMode(pin, OUTPUT);

    tone_pin = pin;
    tone_half_us = 500000UL / frequency;
    tone_cycles_left = (duration > 0) ? (duration * frequency) / 1000 : 0;

    if (!tone_task) {
        xTaskCreate(tone_task_fn, "tone", 2048, NULL, TONE_TASK_PRIORITY,
                    &tone_task);
    }
}

void noTone(uint8_t pin)
{
    if (tone_pin == pin) {
        tone_cycles_left = 0;
        tone_half_us = 0;
        tone_pin = 0xFF;
        digitalWrite(pin, LOW);
    }
}
