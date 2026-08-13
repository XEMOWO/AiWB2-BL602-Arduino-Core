/*
 * util/delay.h — AVR delay shim for the Ai-WB2-12F core.
 *
 * Some libraries (e.g. Adafruit_SSD1306) #include <util/delay.h> and call the
 * AVR-style _delay_ms()/_delay_us(). On BL602 these map straight onto the core
 * delay functions. F_CPU is defined by Arduino.h.
 */
#ifndef _util_delay_h_
#define _util_delay_h_

#include <stdint.h>
#include <Arduino.h>

static inline void _delay_us(double __us)
{
    delayMicroseconds((unsigned int)__us);
}

static inline void _delay_ms(double __ms)
{
    delay((unsigned long)__ms);
}

#endif /* _util_delay_h_ */
