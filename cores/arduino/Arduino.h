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
#include "wiring_private.h" /* direct-GPIO register helpers (OneWire etc.) */

#ifdef __cplusplus
extern "C" {
#endif

/* Arduino data types (minimal subset) */
typedef bool     boolean;
typedef uint8_t  byte;

/* word() / highByte() / lowByte() — byte-combining helpers. These are MACROS
 * in the AVR/ESP cores (NOT a `word` typedef): the NTPClient example and other
 * sketches call word(hi, lo) as a function. A typedef would turn that into a
 * functional cast and break with "expression list treated as compound
 * expression". Same definitions as the ESP8266 core. */
#ifndef word
#define word(high, low) ((high) << 8 | (low))
#endif
#define highByte(w) ((uint8_t) ((w) >> 8))
#define lowByte(w) ((uint8_t) ((w) & 0xff))

/* Bit ordering for shiftOut/shiftIn/SPI — canonical ESP8266/AVR values.
 * (Our own SPI library historically used the opposite numbering; it now uses
 * these constants so third-party code written for ESP8266 behaves identically.) */
typedef enum {
    LSBFIRST = 0,
    MSBFIRST = 1,
} BitOrder;

/* Bit manipulation helpers (standard Arduino, used by Keypad & many libs).
 * Same macro forms as the AVR/ESP cores. */
#define bit(b)          (1UL << (b))
#define bitRead(value, b)      (((value) >> (b)) & 0x01)
#define bitSet(value, b)       ((value) |= (1UL << (b)))
#define bitClear(value, b)     ((value) &= ~(1UL << (b)))
#define bitWrite(value, b, bv) ((bv) ? bitSet(value, b) : bitClear(value, b))

/* lwIP normally exports BIT(n) via lwip/def.h; the SDK's lwip build omits it,
 * and ESP8266 third-party libs (DNSServer) rely on it. Same semantics. */
#ifndef BIT
#define BIT(b) (1UL << (b))
#endif

/* Math/angle constants and helpers (standard Arduino; sq() is used by GPS and
 * math-heavy libs, TWO_PI/HALF_PI by trig callers). */
#define sq(x)           ((x) * (x))
#ifndef PI
#define PI              3.1415926535897932384626433832795
#endif
#ifndef HALF_PI
#define HALF_PI         1.5707963267948966192313216916398
#endif
#ifndef TWO_PI
#define TWO_PI          6.283185307179586476925286766559
#endif

/* Pin modes */
#define INPUT         0x0
#define OUTPUT        0x1
#define INPUT_PULLUP  0x2
#define INPUT_PULLDOWN 0x3

/* Digital states */
#define LOW  0x0
#define HIGH 0x1

/* CPU clock frequency (BL602 runs at 192 MHz). Official cores expose F_CPU in
 * platform.txt; third-party sensor libs (e.g. DHT) read it via
 * microsecondsToClockCycles(). */
#ifndef F_CPU
#define F_CPU 192000000L
#endif

/* ESP8266/ESP32 ISR attributes. No-ops on BL602: code runs from XIP flash, so
 * there is no IRAM speed/placement concern — the macros exist only so sketches
 * that decorate ISRs with ICACHE_RAM_ATTR / IRAM_ATTR still compile. */
#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

/* Convert a duration in microseconds to CPU clock cycles. AVR/ESP cores define
 * this inline in Arduino.h; busy-wait timeout helpers depend on it. */
static inline unsigned long microsecondsToClockCycles(unsigned long micros)
{
    return (micros * (F_CPU / 1000L)) / 1000L;
}

/* Angle conversion helpers — used by display/graphics libraries (e.g.
 * Adafruit_GFX's arc/rotation code calls radians()). Same definitions as the
 * ESP8266 core. */
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.295779513082320876798154814105
#endif
#ifndef radians
#define radians(deg) ((deg)*DEG_TO_RAD)
#endif
#ifndef degrees
#define degrees(rad) ((rad)*RAD_TO_DEG)
#endif

#ifdef __cplusplus
}
#endif

/* ESP8266-core compat headers. Placed after the type typedefs above because
 * WCharacter.h's inline helpers use `boolean`. */
#include "binary.h"     /* B00000000..B11111111 literals */
#include "WCharacter.h" /* isAlpha()/toUpperCase() ctype helpers (inline fns) */
#include "dtostrf.h"    /* char *dtostrf(double, width, prec, char*) */
#include "debug.h"      /* DEBUGV() debug macro (no-op unless DEBUG_ESP_PORT) */
#include "mmu_iram.h"   /* mmu_get_uint16 & friends (XIP direct access) */
#include "esp8266_peri.h" /* GPI/USC0 & friends (BL602 no-op shims) */

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

/* wiring_shift.c — software bit-banged shiftOut/shiftIn (ESP8266 API) */
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);

/* wiring.c */
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
void yield(void);   /* yield to the scheduler (ESP8266 compat) */
void optimistic_yield(uint32_t interval_us); /* rate-limited yield (ESP8266, SdFat) */

/* wiring_analog.c */
int  analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);
int  analogReadMilliVolts(uint8_t pin);
void analogReadResolution(uint8_t bits);   /* 8..16, default 12 */
void analogWriteFrequency(uint32_t freq);  /* Hz, 2000..800000, default 5000 */
void analogWriteRange(uint32_t range);     /* ESP8266: max analogWrite value, default 255 */
void analogWriteFreq(uint32_t freq);       /* ESP8266 alias of analogWriteFrequency */

/* ESP8266-core waveform helper: enables analogWrite on any pin. BL602's PWM
 * is available on every GPIO without a setup call, so this is a no-op kept
 * for source compatibility (FadePolledTimeout calls it). */
void enablePhaseLockedWaveform(void);

/* esp_utils.c */
uint32_t esp_get_free_heap_size(void);
uint32_t getCpuFrequencyMhz(void);
uint32_t esp_get_chip_id(void);
void esp_restart(void);                    /* implemented in wiring_wdg.c */
void noInterrupts(void);
void interrupts(void);
void ets_intr_lock(void);                  /* ESP8266-style IRQ lock/unlock */
void ets_intr_unlock(void);

/* Schedule.cpp — C hooks so wiring.c can service Ticker/Schedule from
 * delay()/yield(). The C++ API (schedule_function & friends) is in Schedule.h. */
int  wb2_sched_pending(void);              /* 1 if any one-shot/recurrent queued */
void wb2_run_scheduled(void);              /* run one-shots + recurrent */
void wb2_run_recurrent(void);              /* run recurrent only */

/* wiring_wdg.c */
void watchdogEnable(uint32_t timeout_ms);
void watchdogDisable(void);
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
void attachInterruptArg(uint8_t pin, void (*userFunc)(void *), void *arg, int mode);
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
/* ESP8266-compatible `ESP` class (getChipId/getFreeHeap/getCycleCount/...).
 * Same placement as the ESP8266 core (Arduino.h includes Esp.h). */
#include "Esp.h"
/* ESP8266 core free functions (esp_yield/esp_get_cycle_count/...). */
#include "coredecls.h"
#include "MD5Builder.h" /* visible globally (ESP8266 gets it via Updater.h) */
/* ESP8266 core exposes Updater.h (Update object, U_FLASH/U_FS) from Arduino.h;
 * ESP8266httpUpdate and the WebUpdate examples rely on it being global. */
#include "Updater.h"

/* ESP8266-core time functions (implemented in time.cpp on the bl_sys_time
 * backend). Same declarations/default args as the ESP8266 core. */
void setTZ(const char* tz);

/* configure time using POSIX TZ string
 * server pointers *must remain valid* for the duration of the program */
void configTime(const char* tz, const char* server1,
    const char* server2 = nullptr, const char* server3 = nullptr);

/* configures with approximated TZ value. part of the old api, prefer
 * configTime with TZ variable */
void configTime(int timezone, int daylightOffset_sec, const char* server1,
    const char* server2 = nullptr, const char* server3 = nullptr);

/* esp32 api compatibility */
inline void configTzTime(const char* tz, const char* server1,
    const char* server2 = nullptr, const char* server3 = nullptr)
{
    configTime(tz, server1, server2, server3);
}

bool getLocalTime(struct tm * info, uint32_t ms = 5000);

/* configTime wrappers for temporary server{1,2,3} strings */
void configTime(int timezone, int daylightOffset_sec, String server1,
    String server2 = String(), String server3 = String());
void configTime(const char* tz, String server1,
    String server2 = String(), String server3 = String());
#endif

/* C++-only conveniences: two-argument tone()/pulseIn() overloads with the
 * usual Arduino defaults, and pulseInLong as an alias. min()/max()/abs()/round()
 * are imported from <algorithm>/<cmath> exactly like the ESP8266 core — NOT
 * defined as macros, so std::min(a,b,comp) and code that includes <algorithm>
 * (e.g. ArduinoJson) still parse. */
#ifdef __cplusplus
#include <algorithm>
#include <cmath>
using std::min;
using std::max;
using std::abs;
/* std::round/isinf/isnan are NOT declared in this freestanding libstdc++/newlib
 * build; code that calls them unqualified resolves to the global ::round() etc.
 * from <math.h> instead — matching ESP8266's observable behavior. */

inline void tone(uint8_t pin, unsigned int frequency) { tone(pin, frequency, 0); }
inline unsigned long pulseIn(uint8_t pin, uint8_t state)
{
    return pulseIn(pin, state, 1000000UL);
}
#define pulseInLong pulseIn

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
