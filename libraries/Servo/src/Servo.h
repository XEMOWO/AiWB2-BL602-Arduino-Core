/*
 * Servo.h — servo control for the Ai-WB2-12F (BL602).
 *
 * API-compatible subset of the Arduino Servo library. BL602's PWM hardware
 * has a 2 kHz minimum frequency, so the 50 Hz (20 ms) servo frame and the
 * 1..2 ms pulses are bit-banged by a shared background task using
 * delayMicroseconds() — microsecond-accurate and independent of the timer
 * used by millis()/timer_begin().
 *
 * All attached Servo objects share that one task; each is pulsed once per
 * 20 ms cycle in attach order. Keep the number of attached servos low
 * (sum of pulse widths must stay well under 20 ms).
 */
#ifndef Servo_h
#define Servo_h

#include <Arduino.h>

#define MAX_SERVOS         8
#define MIN_PULSE_WIDTH    544   /* shortest pulse the library will send, us */
#define MAX_PULSE_WIDTH    2400  /* longest pulse, us                       */
#define DEFAULT_PULSE_WIDTH 1500
#define REFRESH_INTERVAL   20000 /* 50 Hz frame, us */

class Servo
{
public:
    Servo() : _pin(-1), _min(MIN_PULSE_WIDTH), _max(MAX_PULSE_WIDTH),
              _pulseUs(DEFAULT_PULSE_WIDTH), _idx(-1) {}

    /* attach(pin) uses the default 544..2400 us range; the 3-argument form
     * takes a custom min/max (in us) to match a specific servo. */
    uint8_t attach(int pin);
    uint8_t attach(int pin, int min, int max);
    void detach(void);

    void write(int value);            /* angle 0..180 degrees */
    void writeMicroseconds(int value);/* pulse width in us */
    int  read(void) const;            /* last requested angle, 0..180 */
    bool attached(void) const { return _idx >= 0; }

private:
    int8_t _pin;
    int    _min, _max;
    int    _pulseUs;
    int8_t _idx; /* slot in the shared registry, -1 when detached */
};

#endif /* Servo_h */
