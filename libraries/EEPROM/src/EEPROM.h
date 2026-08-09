/*
 * EEPROM.h — EEPROM emulation in the DATA flash partition (BL602).
 *
 * The SDK has no byte-addressable EEPROM; flash writes need an erase-first
 * 4K sector. So this library behaves like the ESP32 core:
 *   - begin(size)  loads a RAM shadow of the first `size` bytes (default 4096,
 *                  one sector, inside the 20 KB DATA partition);
 *   - read()/write()/update() touch the RAM shadow only;
 *   - commit()     erases the sector and writes the shadow to flash.
 * Call commit() before powering off; otherwise writes are lost.
 *
 * get()/put() are provided as templates for arbitrary types (structs, floats).
 */
#ifndef EEPROM_h
#define EEPROM_h

#include <stdint.h>
#include <stddef.h>
#include <string.h>

class EEPROMClass
{
public:
    EEPROMClass() : _data(NULL), _size(0), _dirty(false), _handle(NULL) {}

    void begin(size_t size = 4096);
    void end(void);

    uint8_t read(int addr);
    void write(int addr, uint8_t val);
    uint8_t update(int addr, uint8_t val); /* write only if different */
    bool commit(void);                     /* persist the RAM shadow to flash */
    size_t length(void) const { return _size; }

    template<class T> T &get(int addr, T &t)
    {
        if (_data && addr >= 0 && addr + (int)sizeof(T) <= (int)_size) {
            memcpy(&t, _data + addr, sizeof(T));
        }
        return t;
    }

    template<class T> const T &put(int addr, const T &t)
    {
        if (_data && addr >= 0 && addr + (int)sizeof(T) <= (int)_size) {
            if (memcmp(_data + addr, &t, sizeof(T)) != 0) {
                memcpy(_data + addr, &t, sizeof(T));
                _dirty = true;
            }
        }
        return t;
    }

private:
    uint8_t *_data;
    size_t   _size;
    bool     _dirty;
    void    *_handle; /* bl_mtd_handle_t, opaque here */

    void _load(void);
};

extern EEPROMClass EEPROM;

#endif /* EEPROM_h */
