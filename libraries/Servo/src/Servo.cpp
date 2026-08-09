/*
 * Servo.cpp — shared bit-bang task driving all attached servos.
 *
 * The task loops at 50 Hz: for every attached slot it drives the pin high,
 * waits the servo's pulse width with delayMicroseconds(), then releases.
 * Pulse timing is therefore independent of both the FreeRTOS tick and the
 * timer used by millis(); delayMicroseconds() busy-waits on the hardware
 * counter. Priority 18 sits above the loop() task (15) so frames stay on
 * time even if the sketch is busy, but below tone()'s 20.
 */
#include "Servo.h"

#include <FreeRTOS.h>
#include <task.h>

#define SERVO_TASK_PRIORITY 18
#define SERVO_TASK_STACK    1024

static TaskHandle_t servo_task_handle = NULL;

static volatile int8_t  s_pins[MAX_SERVOS]; /* GPIO per slot, -1 = free */
static volatile uint16_t s_pulse[MAX_SERVOS]; /* pulse width in us */

static void servo_task_fn(void *arg)
{
    (void)arg;
    for (;;) {
        uint8_t i;
        for (i = 0; i < MAX_SERVOS; i++) {
            int8_t p = s_pins[i];
            if (p >= 0) {
                digitalWrite((uint8_t)p, HIGH);
                delayMicroseconds(s_pulse[i]);
                digitalWrite((uint8_t)p, LOW);
            }
        }
        /* ~50 Hz refresh; vTaskDelay keeps the tick accurate enough here */
        vTaskDelay(pdMS_TO_TICKS(REFRESH_INTERVAL / 1000));
    }
}

static void servo_ensure_task(void)
{
    if (!servo_task_handle) {
        xTaskCreate(servo_task_fn, "servo", SERVO_TASK_STACK, NULL,
                    SERVO_TASK_PRIORITY, &servo_task_handle);
    }
}

uint8_t Servo::attach(int pin)
{
    return attach(pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
}

uint8_t Servo::attach(int pin, int min, int max)
{
    int8_t slot = -1;
    uint8_t i;

    if (pin < 0 || pin >= NUM_DIGITAL_PINS) {
        return 0;
    }
    /* Reuse our own slot if already attached (re-attach). */
    if (_idx >= 0) {
        slot = _idx;
    } else {
        for (i = 0; i < MAX_SERVOS; i++) {
            if (s_pins[i] < 0) {
                slot = (int8_t)i;
                break;
            }
        }
        if (slot < 0) {
            return 0; /* no free slot */
        }
    }

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    _pin = pin;
    _min = min;
    _max = max;
    _pulseUs = DEFAULT_PULSE_WIDTH;
    _idx = slot;

    s_pins[slot] = (int8_t)pin;
    s_pulse[slot] = (uint16_t)_pulseUs;

    servo_ensure_task();
    return 1;
}

void Servo::detach(void)
{
    if (_idx < 0) {
        return;
    }
    s_pins[_idx] = -1;
    s_pulse[_idx] = 0;
    _idx = -1;
    _pin = -1;
}

void Servo::write(int value)
{
    if (_idx < 0) {
        return;
    }
    if (value < 0) {
        value = 0;
    } else if (value > 180) {
        value = 180;
    }
    writeMicroseconds(_min + (int32_t)value * (_max - _min) / 180);
}

void Servo::writeMicroseconds(int value)
{
    if (_idx < 0) {
        return;
    }
    if (value < _min) {
        value = _min;
    } else if (value > _max) {
        value = _max;
    }
    _pulseUs = value;
    s_pulse[_idx] = (uint16_t)value;
}

int Servo::read(void) const
{
    if (_idx < 0 || _max == _min) {
        return 0;
    }
    return (_pulseUs - _min) * 180 / (_max - _min);
}
