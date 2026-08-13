/*
  EEPROM.cpp - EEPROM emulation for the Ai-WB2-12F (BL602)

  API surface is 100% the ESP8266 core's EEPROM library (see EEPROM.h), so
  ESP8266 sketches and third-party libraries compile and run unchanged.

  Backend: the "DATA" boot2 partition (flash 0x1F3000, 20 KB = 5 x 4 KB
  sectors in the 2M layout) is used with a 4-sector, sequence-numbered
  rotation so repeated commit() calls wear the flash evenly:
    - each sector is 4096 B: the first 4088 B hold user data, the last 8 B
      hold a header (magic "AEEP" + 32-bit monotonic sequence number);
    - begin() scans the 4 sectors and loads the one with the highest
      sequence number into a RAM shadow; if no valid header exists (fresh
      EEPROM), the shadow is filled with 0xFF so first-run detection via
      read() == 0xFF behaves like a never-used ESP8266 sector;
    - commit() erases the sector next to the active one, writes the shadow
      and the next header, and advances the active index;
    - on sequence-number wraparound (2^32 commits) all four sectors are
      re-erased and sector 0 becomes active again.
  If the DATA partition cannot be opened, begin() still allocates the RAM
  shadow (reads come back 0xFF) and commit() reports failure -- a sketch
  that never commits keeps working.

  bl_mtd.h has no extern "C" guards -- wrap it here.

  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"
#include "EEPROM.h"
#include "debug.h"

extern "C" {
#include "bl_mtd.h"
}

#define EEPROM_SECTOR_SIZE     4096      /* BL602 flash erase sector */
#define EEPROM_SECTOR_COUNT    4         /* rotation pool inside DATA */
#define EEPROM_DATA_SIZE       (EEPROM_SECTOR_SIZE - 8) /* 4088 B user data */
#define EEPROM_MAGIC           "AEEP"

struct EepromHeader {
    uint8_t  magic[4];  /* EEPROM_MAGIC */
    uint32_t seq;       /* monotonic, little-endian */
};

/* Lazy singleton handle to the DATA partition (never closed). */
static bl_mtd_handle_t s_handle = NULL;
/* Active rotation sector / its sequence number; -1 = not scanned yet. */
static int      s_activeSector = -1;
static uint32_t s_activeSeq    = 0;

static bl_mtd_handle_t eepromOpen(void)
{
    if (!s_handle) {
        bl_mtd_handle_t h = NULL;
        if (bl_mtd_open("DATA", &h, BL_MTD_OPEN_FLAG_NONE) == 0) {
            s_handle = h;
        }
    }
    return s_handle;
}

/* Read a sector's header. Returns UINT32_MAX when the magic is wrong or the
 * read failed (treated as "not a valid rotation slot"). */
static uint32_t eepromReadSeq(bl_mtd_handle_t h, uint32_t sectorIdx)
{
    EepromHeader hdr;
    uint32_t off = sectorIdx * EEPROM_SECTOR_SIZE + EEPROM_DATA_SIZE;
    if (bl_mtd_read(h, off, sizeof(hdr), (uint8_t *) &hdr) != 0) {
        return UINT32_MAX;
    }
    if (memcmp(hdr.magic, EEPROM_MAGIC, 4) != 0) {
        return UINT32_MAX;
    }
    return hdr.seq;
}

/* Find the sector holding the most recent commit. Returns -1 if none valid. */
static int eepromFindActive(bl_mtd_handle_t h)
{
    uint32_t bestSeq = 0;
    int best = -1;
    for (int i = 0; i < EEPROM_SECTOR_COUNT; ++i) {
        uint32_t seq = eepromReadSeq(h, i);
        if (seq == UINT32_MAX) {
            continue; /* erased or corrupt slot */
        }
        if (best < 0 || seq > bestSeq) {
            best = i;
            bestSeq = seq;
        }
    }
    return best;
}

/* Load `size` bytes of the active sector into buf. Returns false when the
 * pool has no valid commit yet (caller FF-fills the shadow). */
static bool eepromLoad(uint8_t *buf, size_t size)
{
    bl_mtd_handle_t h = eepromOpen();
    if (!h) {
        return false;
    }
    if (s_activeSector < 0) {
        s_activeSector = eepromFindActive(h);
        if (s_activeSector < 0) {
            return false; /* fresh EEPROM */
        }
        s_activeSeq = eepromReadSeq(h, s_activeSector);
    }
    return bl_mtd_read(h, s_activeSector * EEPROM_SECTOR_SIZE, size, buf) == 0;
}

EEPROMClass::EEPROMClass(uint32_t sector)
: _sector(sector)
{
}

EEPROMClass::EEPROMClass(void)
: _sector(0)
{
}

void EEPROMClass::begin(size_t size) {
  if (size <= 0) {
    DEBUGV("EEPROMClass::begin error, size == 0\n");
    return;
  }
  if (size > EEPROM_DATA_SIZE) {
    DEBUGV("EEPROMClass::begin error, %d > %d\n", size, EEPROM_DATA_SIZE);
    size = EEPROM_DATA_SIZE;
  }

  size = (size + 3) & (~3);

  //In case begin() is called a 2nd+ time, don't reallocate if size is the same
  if(_data && size != _size) {
    delete[] _data;
    _data = new uint8_t[size];
  } else if(!_data) {
    _data = new uint8_t[size];
  }

  _size = size;

  if (!eepromLoad(_data, _size)) {
    /* fresh / DATA partition unavailable: present erased-flash state so
       first-run detection (read() == 0xFF) matches a never-used ESP8266. */
    memset(_data, 0xFF, _size);
  }

  _dirty = false; //make sure dirty is cleared in case begin() is called 2nd+ time
}

bool EEPROMClass::end() {
  bool retval;

  if(!_size) {
    return false;
  }

  retval = commit();
  if(_data) {
    delete[] _data;
  }
  _data = 0;
  _size = 0;
  _dirty = false;

  return retval;
}


uint8_t EEPROMClass::read(int const address) {
  if (address < 0 || (size_t)address >= _size) {
    DEBUGV("EEPROMClass::read error, address %d > %d or %d < 0\n", address, _size, address);
    return 0;
  }
  if (!_data) {
    DEBUGV("EEPROMClass::read without ::begin\n");
    return 0;
  }

  return _data[address];
}

void EEPROMClass::write(int const address, uint8_t const value) {
  if (address < 0 || (size_t)address >= _size) {
    DEBUGV("EEPROMClass::write error, address %d > %d or %d < 0\n", address, _size, address);
    return;
  }
  if(!_data) {
    DEBUGV("EEPROMClass::write without ::begin\n");
    return;
  }

  // Optimise _dirty. Only flagged if data written is different.
  uint8_t* pData = &_data[address];
  if (*pData != value)
  {
    *pData = value;
    _dirty = true;
  }
}

bool EEPROMClass::commit() {
  if (!_size)
    return false;
  if(!_dirty)
    return true;
  if(!_data)
    return false;

  bl_mtd_handle_t h = eepromOpen();
  if (!h)
    return false;

  uint32_t nextSeq;
  int nextSector;

  if (s_activeSector < 0) {
    s_activeSector = eepromFindActive(h);
    if (s_activeSector < 0) {
      /* fresh EEPROM: seed the rotation from sector 0, seq 0 */
      s_activeSector = 0;
      s_activeSeq = 0;
      nextSector = 0;
      nextSeq = 1;
    } else {
      s_activeSeq = eepromReadSeq(h, s_activeSector);
      nextSector = (s_activeSector + 1) % EEPROM_SECTOR_COUNT;
      nextSeq = s_activeSeq + 1;
    }
  } else {
    nextSector = (s_activeSector + 1) % EEPROM_SECTOR_COUNT;
    nextSeq = s_activeSeq + 1;
  }

  if (nextSeq == 0) {
    /* wraparound after 2^32 commits: re-erase the whole pool, restart at 0 */
    for (int i = 0; i < EEPROM_SECTOR_COUNT; ++i) {
      if (bl_mtd_erase(h, i * EEPROM_SECTOR_SIZE, EEPROM_SECTOR_SIZE) != 0) {
        return false;
      }
    }
    nextSector = 0;
  }

  if (bl_mtd_erase(h, nextSector * EEPROM_SECTOR_SIZE, EEPROM_SECTOR_SIZE) != 0) {
    return false;
  }

  if (bl_mtd_write(h, nextSector * EEPROM_SECTOR_SIZE, _size, _data) != 0) {
    return false;
  }

  EepromHeader hdr;
  memcpy(hdr.magic, EEPROM_MAGIC, 4);
  hdr.seq = nextSeq;
  if (bl_mtd_write(h, nextSector * EEPROM_SECTOR_SIZE + EEPROM_DATA_SIZE,
                   sizeof(hdr), (const uint8_t *) &hdr) != 0) {
    return false;
  }

  s_activeSector = nextSector;
  s_activeSeq = nextSeq;
  _dirty = false;
  return true;
}

uint8_t * EEPROMClass::getDataPtr() {
  _dirty = true;
  return &_data[0];
}

uint8_t const * EEPROMClass::getConstDataPtr() const {
  return &_data[0];
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
EEPROMClass EEPROM;
#endif
