/*
 * SoftwareSerial.cpp — task-based bit-bang UART.
 *
 * RX: the task waits for the start bit (falling edge) by polling the pin,
 * yields via taskYIELD() while idle, then samples the 8 data bits at
 * mid-bit using micros()-based deadlines. A 64-byte ring buffer decouples
 * the sampling task from the consumer sketch.
 *
 * TX: write() builds the frame with the same micros() deadlines. The pin
 * idles HIGH; the start bit pulls it LOW.
 *
 * micros() returns unsigned long (32 bit). All waits use the wrap-safe
 * comparison "(int32_t)(micros() - target) < 0", which is true while the
 * target time is still in the future.
 */
#include "SoftwareSerial.h"

#define SW_SERIAL_TASK_PRIORITY 18 /* above loop() so frames stay in time */
#define SW_SERIAL_TASK_STACK    2048
#define SW_SERIAL_IDLE_WAIT_MS  10 /* task sleep when the line is quiet */

static inline uint8_t sw_next(uint8_t i)
{
    return (uint8_t)((i + 1) & (SW_SERIAL_RX_BUFFER_SIZE - 1));
}

void SoftwareSerial::begin(long speed)
{
    if (speed <= 0) {
        return;
    }
    _bitTimeUs = 1000000UL / (uint32_t)speed;
    _head = _tail = 0;
    _overflow = false;

    pinMode(_rxPin, INPUT_PULLUP); /* idle HIGH for UART */
    pinMode(_txPin, OUTPUT);
    digitalWrite(_txPin, HIGH);

    _listening = true;
    if (!_task) {
        xTaskCreate(_taskFn, "swserial", SW_SERIAL_TASK_STACK, this,
                    SW_SERIAL_TASK_PRIORITY, &_task);
    }
}

void SoftwareSerial::end(void)
{
    _listening = false;
    /* keep the task around; begin() reuses it */
}

void SoftwareSerial::_taskFn(void *arg)
{
    SoftwareSerial *s = (SoftwareSerial *)arg;
    for (;;) {
        if (s->_listening) {
            s->_rxLoop();
        } else {
            vTaskDelay(pdMS_TO_TICKS(SW_SERIAL_IDLE_WAIT_MS));
        }
    }
}

/* Blocking sample of one frame. Returns nothing; the byte (or an overrun)
 * lands in the ring buffer. */
void SoftwareSerial::_rxLoop(void)
{
    uint32_t t0;
    uint32_t mid;
    uint8_t c = 0;
    uint8_t i;

    /* Wait for the start bit. Polling keeps us within ~1 bus cycle of the
     * falling edge; taskYIELD() lets the loop task run while idle. If the
     * line stays high for a while, sleep in chunks to save CPU. */
    t0 = micros();
    while (digitalRead(_rxPin)) {
        if ((uint32_t)(micros() - t0) >= 200000UL) { /* 200 ms quiet */
            vTaskDelay(pdMS_TO_TICKS(SW_SERIAL_IDLE_WAIT_MS));
            t0 = micros();
            continue;
        }
        taskYIELD();
    }

    /* Start bit seen. Sample each data bit at its mid-point. */
    mid = micros() + _bitTimeUs / 2;
    for (i = 0; i < 8; i++) {
        uint32_t target = mid + (uint32_t)i * _bitTimeUs;
        while ((int32_t)(micros() - target) < 0) {
            /* busy wait */
        }
        c >>= 1;
        if (digitalRead(_rxPin)) {
            c |= 0x80;
        }
    }

    /* Push to ring (drop on overrun but keep the oldest byte). */
    if (sw_next(_head) != _tail) {
        _rxbuf[_head] = c;
        _head = sw_next(_head);
    } else {
        _overflow = true;
    }
}

int SoftwareSerial::available(void)
{
    return (int)((_head - _tail) & (SW_SERIAL_RX_BUFFER_SIZE - 1));
}

int SoftwareSerial::peek(void)
{
    if (_head == _tail) {
        return -1;
    }
    return _rxbuf[_tail];
}

int SoftwareSerial::read(void)
{
    if (_head == _tail) {
        return -1;
    }
    uint8_t c = _rxbuf[_tail];
    _tail = sw_next(_tail);
    return c;
}

size_t SoftwareSerial::write(uint8_t byte)
{
    uint32_t start = micros();
    uint8_t b = byte;
    uint8_t i;

    pinMode(_txPin, OUTPUT);

    /* start bit (LOW) */
    digitalWrite(_txPin, LOW);
    for (i = 0; i < 8; i++) {
        uint32_t target = start + (uint32_t)(i + 1) * _bitTimeUs;
        while ((int32_t)(micros() - target) < 0) {
            /* busy wait */
        }
        digitalWrite(_txPin, (b & 0x01) ? HIGH : LOW);
        b >>= 1;
    }
    /* stop bit (HIGH) */
    {
        uint32_t target = start + 9 * _bitTimeUs;
        while ((int32_t)(micros() - target) < 0) {
            /* busy wait */
        }
        digitalWrite(_txPin, HIGH);
        target = start + 10 * _bitTimeUs;
        while ((int32_t)(micros() - target) < 0) {
            /* busy wait */
        }
    }
    return 1;
}
