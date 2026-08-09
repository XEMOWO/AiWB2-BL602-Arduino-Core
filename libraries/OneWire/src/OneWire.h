/*
 * OneWire.h — Dallas 1-Wire bus on the Ai-WB2-12F (BL602).
 *
 * Bit-banged on any GPIO as an open-drain bus: the pin is driven low for
 * logical 0 and released (hi-Z with internal pull-up) for logical 1.
 * Timing uses delayMicroseconds(), which busy-waits on the hardware counter,
 * so reads/writes are not disturbed by FreeRTOS scheduling.
 *
 * The API follows the classic Arduino OneWire library, including the ROM
 * search() that enumerates all devices on the bus.
 */
#ifndef OneWire_h
#define OneWire_h

#include <Arduino.h>

class OneWire
{
public:
    OneWire(uint8_t pin) : _pin(pin) { _rom[0] = 0; }

    void begin(void); /* configure the pin (input + pull-up) */

    /* bus */
    bool reset(void);                 /* returns true if a device answered */
    void depower(void) {}             /* nothing to do: no external pull-up */
    void select(const uint8_t rom[8]);
    void skip(void);

    /* bit-level */
    void write_bit(uint8_t v);
    uint8_t read_bit(void);

    /* byte-level */
    void write(uint8_t v, int power = 0); /* power is ignored (no parasitic) */
    uint8_t read(void);
    void write_bytes(const uint8_t *buf, uint16_t count);
    uint8_t read_bytes(uint8_t *buf, uint16_t count);

    /* ROM search */
    void reset_search(void);
    bool search(uint8_t *newAddr, bool search_mode = true);

    static uint8_t crc8(const uint8_t *addr, uint8_t len);
    static uint16_t crc16(const uint16_t *data, uint16_t len, uint16_t crc = 0);

private:
    uint8_t _pin;
    uint8_t _rom[8];
    uint8_t _lastDiscrepancy;
    uint8_t _lastDeviceFlag;
    uint8_t _lastFamilyDiscrepancy;
};

#endif /* OneWire_h */
