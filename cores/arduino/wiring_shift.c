/*
 * wiring_shift.c — software bit-banged shiftOut()/shiftIn() (ESP8266 API).
 *
 * Generic bit-banged implementation used by shift-register and LED-display
 * libraries (74HC595, MAX7219, ...). Same semantics as the ESP8266 core.
 */
#include "Arduino.h"

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        uint8_t bit = (bitOrder == LSBFIRST) ? (1 << i) : (1 << (7 - i));
        digitalWrite(dataPin, (val & bit) ? HIGH : LOW);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
{
    uint8_t value = 0;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        if (bitOrder == LSBFIRST) {
            value |= (uint8_t)digitalRead(dataPin) << i;
        } else {
            value |= (uint8_t)digitalRead(dataPin) << (7 - i);
        }
        digitalWrite(clockPin, LOW);
    }
    return value;
}
