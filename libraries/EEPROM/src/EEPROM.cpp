/*
 * EEPROM.cpp — EEPROM on the bl_mtd DATA partition.
 *
 * The DATA partition is 20 KB at flash 0x1F3000 (2M layout); the default
 * 4096-byte region is one 4K sector, kept clear of the firmware image.
 * bl_mtd_write() is program-only (can only clear bits), so commit() always
 * erases the sector first.
 *
 * bl_mtd.h has no extern "C" guards — wrap it here.
 */
#include "EEPROM.h"

#include <stdlib.h>
#include <Arduino.h>

extern "C" {
#include <bl_mtd.h>
}

EEPROMClass EEPROM;

void EEPROMClass::_load(void)
{
    bl_mtd_info_t info;
    size_t sz;

    if (_data || !_handle) {
        return;
    }
    if (bl_mtd_info(_handle, &info) != 0) {
        return;
    }
    sz = info.size;                    /* 0x5000 = 20 KB */
    if (sz > _size) {
        sz = _size;
    }
    _data = (uint8_t *)malloc(sz ? sz : 1);
    if (!_data) {
        return;
    }
    bl_mtd_read(_handle, 0, sz, _data);
    _size = sz;
    _dirty = false;
}

void EEPROMClass::begin(size_t size)
{
    if (!_handle) {
        bl_mtd_handle_t h = NULL;
        if (bl_mtd_open("DATA", &h, BL_MTD_OPEN_FLAG_NONE) != 0) {
            return; /* partition unavailable; reads/writes become no-ops */
        }
        _handle = h;
    }
    _size = size;
    _load();
}

void EEPROMClass::end(void)
{
    if (_handle) {
        bl_mtd_close(_handle);
        _handle = NULL;
    }
    if (_data) {
        free(_data);
        _data = NULL;
    }
    _size = 0;
    _dirty = false;
}

uint8_t EEPROMClass::read(int addr)
{
    return (_data && addr >= 0 && (size_t)addr < _size) ? _data[addr] : 0xFF;
}

void EEPROMClass::write(int addr, uint8_t val)
{
    if (_data && addr >= 0 && (size_t)addr < _size && _data[addr] != val) {
        _data[addr] = val;
        _dirty = true;
    }
}

uint8_t EEPROMClass::update(int addr, uint8_t val)
{
    if (read(addr) != val) {
        write(addr, val);
    }
    return read(addr);
}

bool EEPROMClass::commit(void)
{
    if (!_data || !_handle || !_dirty) {
        return false;
    }
    if (bl_mtd_erase(_handle, 0, _size) != 0) {
        return false;
    }
    if (bl_mtd_write(_handle, 0, _size, _data) != 0) {
        return false;
    }
    _dirty = false;
    return true;
}
