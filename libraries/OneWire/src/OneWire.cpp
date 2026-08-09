/*
 * OneWire.cpp — 1-Wire bit timing.
 *
 * Bus timing (standard Dallas, nominal; margins are generous):
 *   reset:  drive LOW 480 us, release, 70 us later sample presence, 410 us rest
 *   write 0: drive LOW 60 us, release, 10 us recovery
 *   write 1: drive LOW  6 us, release, 64 us recovery
 *   read:    drive LOW  6 us, release,  9 us later sample, 55 us recovery
 * A slot is 60..120 us with >=1 us recovery; delayMicroseconds() covers all.
 *
 * Open-drain is emulated with bl_gpio: enable_output+set LOW to drive, or
 * enable_input(pullup) to release. bl_gpio.h has no extern "C" guard — wrap.
 */
#include "OneWire.h"

#include <string.h>

extern "C" {
#include <bl_gpio.h>
}

/* ---- low-level open-drain helpers ---- */

static inline void ow_drive_low(uint8_t pin)
{
    bl_gpio_enable_output(pin, 0, 0);
    bl_gpio_output_set(pin, 0);
}

static inline void ow_release(uint8_t pin)
{
    bl_gpio_enable_input(pin, 1, 0); /* hi-Z, internal pull-up */
}

static inline int ow_read(uint8_t pin)
{
    return bl_gpio_input_get_value(pin);
}

void OneWire::begin(void)
{
    ow_release(_pin);
}

bool OneWire::reset(void)
{
    int presence = 1;

    ow_drive_low(_pin);
    delayMicroseconds(480);
    ow_release(_pin);
    delayMicroseconds(70); /* wait for a device to pull the line low */
    presence = ow_read(_pin);
    delayMicroseconds(410); /* rest of the reset slot */
    return (presence == 0);
}

void OneWire::write_bit(uint8_t v)
{
    if (v & 1) {
        ow_drive_low(_pin);
        delayMicroseconds(6);
        ow_release(_pin);
        delayMicroseconds(64);
    } else {
        ow_drive_low(_pin);
        delayMicroseconds(60);
        ow_release(_pin);
        delayMicroseconds(10);
    }
}

uint8_t OneWire::read_bit(void)
{
    uint8_t v = 0;

    ow_drive_low(_pin);
    delayMicroseconds(6);
    ow_release(_pin);
    delayMicroseconds(9);
    v = ow_read(_pin) ? 1 : 0;
    delayMicroseconds(55);
    return v;
}

void OneWire::write(uint8_t v, int power)
{
    (void)power;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        write_bit(v & 0x01);
        v >>= 1;
    }
}

uint8_t OneWire::read(void)
{
    uint8_t v = 0;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        v >>= 1;
        if (read_bit()) {
            v |= 0x80;
        }
    }
    return v;
}

void OneWire::write_bytes(const uint8_t *buf, uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++) {
        write(buf[i]);
    }
}

uint8_t OneWire::read_bytes(uint8_t *buf, uint16_t count)
{
    uint16_t i;
    uint8_t crc = 0;

    for (i = 0; i < count; i++) {
        buf[i] = read();
        crc = crc8(&buf[i], 1); /* maintain running CRC over what we read */
    }
    return crc;
}

void OneWire::select(const uint8_t rom[8])
{
    uint8_t i;

    write(0x55); /* MATCH ROM */
    for (i = 0; i < 8; i++) {
        write(rom[i]);
    }
}

void OneWire::skip(void)
{
    write(0xCC); /* SKIP ROM */
}

void OneWire::reset_search(void)
{
    _lastDiscrepancy = 0;
    _lastDeviceFlag = false;
    _lastFamilyDiscrepancy = 0;
}

/*
 * Classic Dallas ROM search. On the first call (after reset_search) the
 * bus is enumerated bit by bit: each 1-Wire device echoes both its bit and
 * its complement, and whenever a bit position shows a 0/0 discrepancy the
 * search picks a branch (0 first, then 1 on the next pass). Repeating the
 * call walks every ROM present.
 */
bool OneWire::search(uint8_t *newAddr, bool search_mode)
{
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t id_bit, cmp_id_bit;
    uint8_t rom_byte_mask = 1;
    bool search_result = false;
    uint8_t i;

    if (!_lastDeviceFlag) {
        if (!reset()) {
            _lastDiscrepancy = 0;
            _lastDeviceFlag = false;
            _lastFamilyDiscrepancy = 0;
            return false;
        }

        write(search_mode ? 0xF0 : 0xEC); /* SEARCH / CONDITIONAL SEARCH */

        do {
            id_bit = read_bit();
            cmp_id_bit = read_bit();

            if (id_bit && cmp_id_bit) {
                break; /* no devices hold this bit position */
            }

            uint8_t search_direction;
            if (id_bit != cmp_id_bit) {
                search_direction = id_bit; /* every device agrees */
            } else {
                /* discrepancy: pick the branch consistent with previous passes */
                if (id_bit_number < _lastDiscrepancy) {
                    search_direction =
                        (_rom[rom_byte_number] & rom_byte_mask) ? 1 : 0;
                } else {
                    search_direction = (id_bit_number == _lastDiscrepancy);
                }
                if (search_direction == 0) {
                    last_zero = id_bit_number;
                    if (last_zero < 9) {
                        _lastFamilyDiscrepancy = last_zero;
                    }
                }
            }

            if (search_direction) {
                _rom[rom_byte_number] |= rom_byte_mask;
            } else {
                _rom[rom_byte_number] &= (uint8_t)~rom_byte_mask;
            }
            write_bit(search_direction);

            id_bit_number++;
            rom_byte_mask <<= 1;
            if (rom_byte_mask == 0) {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
        } while (rom_byte_number < 8);

        if (id_bit_number > 64) { /* all 64 ROM bits were read */
            _lastDiscrepancy = last_zero;
            if (_lastDiscrepancy == 0) {
                _lastDeviceFlag = true;
            }
            search_result = true;
        }
    }

    if (!search_result || !_rom[0]) {
        /* no device found (or a zero family byte) — reset the search */
        _lastDiscrepancy = 0;
        _lastDeviceFlag = false;
        _lastFamilyDiscrepancy = 0;
        search_result = false;
    } else {
        for (i = 0; i < 8; i++) {
            newAddr[i] = _rom[i];
        }
    }
    return search_result;
}

uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len)
{
    uint8_t crc = 0;

    while (len--) {
        uint8_t inbyte = *addr++;
        uint8_t j;
        for (j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

uint16_t OneWire::crc16(const uint16_t *data, uint16_t len, uint16_t crc)
{
    while (len--) {
        uint16_t v = *data++;
        uint8_t i;
        for (i = 0; i < 8; i++) {
            uint16_t mix = ((crc & 0x8000) >> 8) ^ (v & 0x80);
            crc <<= 1;
            if (mix & 0x80) {
                crc ^= 0x8005;
            }
            v <<= 1;
        }
    }
    return crc;
}
