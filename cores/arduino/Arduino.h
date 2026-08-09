/*
 * Arduino.h — master header for the Ai-WB2-12F (BL602) Arduino core.
 *
 * Minimal Arduino API implementation on top of the Ai-Thinker WB2 SDK
 * (bl_iot_sdk). This file, together with wiring_*.c / main.cpp, forms a
 * standard Arduino board package; the same sources also build as an SDK
 * component for command-line verification.
 */
#ifndef Arduino_h
#define Arduino_h

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>   /* printf: routed to UART0 by the SDK's stdio console */

#include "pgmspace.h" /* PROGMEM / F() / PSTR — no-ops on BL602 (flash XIP) */

#ifdef __cplusplus
extern "C" {
#endif

/* Arduino data types (minimal subset) */
typedef bool     boolean;
typedef uint8_t  byte;
typedef uint16_t word;

/* Pin modes */
#define INPUT         0x0
#define OUTPUT        0x1
#define INPUT_PULLUP  0x2
#define INPUT_PULLDOWN 0x3

/* Digital states */
#define LOW  0x0
#define HIGH 0x1

/* Math helpers in macro form, like the AVR/ESP cores. The SDK headers do not
 * use bare min()/max() identifiers, so the macros are safe to define. */
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifdef __cplusplus
}
#endif

/* Pin mapping for the selected board (variants/<board>/pins_arduino.h).
 * The build system puts the variant dir on the include path. */
#include "pins_arduino.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 14
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* wiring_digital.c */
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int  digitalRead(uint8_t pin);

/* wiring.c */
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

/* wiring_analog.c */
int  analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);
int  analogReadMilliVolts(uint8_t pin);
void analogReadResolution(uint8_t bits);   /* 8..16, default 12 */
void analogWriteFrequency(uint32_t freq);  /* Hz, 2000..800000, default 5000 */

/* esp_utils.c */
uint32_t esp_get_free_heap_size(void);
uint32_t getCpuFrequencyMhz(void);
uint32_t esp_get_chip_id(void);
void esp_restart(void);                    /* implemented in wiring_wdg.c */
void noInterrupts(void);
void interrupts(void);

/* wiring_wdg.c */
void watchdogEnable(uint32_t timeout_ms);
void watchdogFeed(void);

/* wiring_timer.c — one hardware timer, ISR callback */
void timer_begin(uint32_t period_us, void (*cb)(void));
void timer_set_period(uint32_t period_us);
void timer_start(void);
void timer_stop(void);

/* wiring_rtc.c — Unix epoch seconds */
void rtc_set_time(uint32_t sec);
uint32_t rtc_get_time(void);

/* Interrupt trigger modes (standard Arduino values; LOW/HIGH reuse the
 * digital-state macros above). */
#define CHANGE  0x2
#define FALLING 0x3
#define RISING  0x4

/* wiring_interrupt.c */
void attachInterrupt(uint8_t pin, void (*userFunc)(void), int mode);
void detachInterrupt(uint8_t pin);

/* wiring_tone.c / wiring_pulse.c. The 2-argument forms (default duration /
 * timeout) are provided as C++ inline overloads below. */
void tone(uint8_t pin, unsigned int frequency, unsigned long duration);
void noTone(uint8_t pin);
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout);

/* entry glue: called from the SDK's main() (C source) */
void arduino_main(void);

#ifdef __cplusplus
}
#endif

/* Serial is C++ only (class + ring buffer); keep C translation units clean. */
#ifdef __cplusplus
#include "HardwareSerial.h"
#endif

/* C++-only conveniences: two-argument tone()/pulseIn() overloads with the
 * usual Arduino defaults, and pulseInLong as an alias. abs() is provided as an
 * inline overload of the POSIX abs(int) from newlib's <stdlib.h>. */
#ifdef __cplusplus
inline void tone(uint8_t pin, unsigned int frequency) { tone(pin, frequency, 0); }
inline unsigned long pulseIn(uint8_t pin, uint8_t state)
{
    return pulseIn(pin, state, 1000000UL);
}
#define pulseInLong pulseIn

/* abs() needs no definition here: on this target int == long == 32 bit, and
 * libstdc++ already exposes abs(int/long/float/double) through <stdlib.h>. */

/* WMath (C++ linkage: our random(long) overloads newlib's random(void)) */
long map(long x, long in_min, long in_max, long out_min, long out_max);
long constrain(long amt, long low, long high);
void randomSeed(unsigned long seed);
long random(long howbig);
long random(long howsmall, long howbig);
#endif

/* Sketch entry points, defined by the user's .ino / sketch .cpp */
void setup(void);
void loop(void);

#endif /* Arduino_h */
