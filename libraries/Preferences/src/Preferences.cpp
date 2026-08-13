/*
 * Preferences.cpp — key-value table over the bl_mtd DATA partition.
 *
 * Layout of the region (sector 1 of the 20 KB DATA partition):
 *   [4B magic "PREF"][32B namespace, NUL-terminated][2B reserved]
 *   then a sequence of entries:
 *     [1B type][1B key_len][key bytes][2B data_len LE][data bytes]
 *   terminated by an all-zero 2-byte header (type 0, key_len 0).
 *   type 0xFF marks a deleted entry; compaction reclaims it.
 *
 * On BL602 flash an erased byte reads 0xFF, so a never-written region is
 * detected by a missing magic and initialized with a fresh header.
 *
 * bl_mtd.h has no extern "C" guards — wrap it here.
 */
#include "Preferences.h"

#include <stdlib.h>
#include <string.h>
#include <Arduino.h>

extern "C" {
#include <bl_mtd.h>
}

/* The DATA partition (20 KB at flash 0x1F3000) has 5 sectors; sector 0 is
 * owned by the EEPROM library, sector 1 is used here. Both are 4K-aligned. */
#define PREFS_REGION_OFFSET 0x1000
#define PREFS_REGION_SIZE   0x1000
#define PREFS_MAGIC         0x50524546UL /* 'PREF' */
#define PREFS_NS_MAX        32
#define PREFS_HDR_SIZE      (4 + PREFS_NS_MAX + 2) /* magic + ns + reserved */
#define PREFS_NS_STORE      PREFS_HDR_SIZE

static uint8_t s_buf[PREFS_REGION_SIZE]; /* RAM shadow of the region */

/* File-scope type marker for the free helper functions below; the class also
 * declares the same constants for its own use (see Preferences.h). */
enum { PREFS_TYPE_DELETED = 0xFF };

Preferences::Preferences() : _handle(NULL), _readOnly(false), _dirty(false)
{
    _ns[0] = '\0';
}

/* Locate an entry. Returns true and fills data_len/data when found.
 * want_type 0 matches any type. */
static bool prefs_find(const char *key, uint8_t want_type,
                       uint16_t *data_len, const uint8_t **data)
{
    uint16_t p = PREFS_NS_STORE;
    size_t klen = strlen(key);

    while (p + 2 <= PREFS_REGION_SIZE) {
        uint8_t type = s_buf[p];
        uint8_t klen8 = s_buf[p + 1];
        if (type == 0 && klen8 == 0) {
            break; /* end marker */
        }
        uint16_t dlen = (uint16_t)(s_buf[p + 2 + klen8]) |
                        ((uint16_t)s_buf[p + 3 + klen8] << 8);
        if (type != PREFS_TYPE_DELETED && klen == (size_t)klen8 &&
            memcmp(&s_buf[p + 2], key, klen) == 0 &&
            (want_type == 0 || type == want_type)) {
            if (data_len) {
                *data_len = dlen;
            }
            if (data) {
                *data = &s_buf[p + 4 + klen8];
            }
            return true;
        }
        p += 4 + klen8 + dlen;
    }
    return false;
}

void Preferences::_reset(const char *name)
{
    size_t nslen = strlen(name);
    uint32_t magic = PREFS_MAGIC;

    memset(s_buf, 0xFF, PREFS_REGION_SIZE);
    memcpy(s_buf, &magic, 4);
    if (nslen > PREFS_NS_MAX - 1) {
        nslen = PREFS_NS_MAX - 1;
    }
    memcpy(&s_buf[4], name, nslen);
    s_buf[4 + nslen] = 0;
    s_buf[PREFS_HDR_SIZE] = 0; /* end marker */
    s_buf[PREFS_HDR_SIZE + 1] = 0;
}

uint16_t Preferences::_tableEnd(void)
{
    uint16_t p = PREFS_NS_STORE;
    while (p + 2 <= PREFS_REGION_SIZE) {
        uint8_t type = s_buf[p];
        uint8_t klen = s_buf[p + 1];
        if (type == 0 && klen == 0) {
            return p;
        }
        uint16_t dlen = (uint16_t)(s_buf[p + 2 + klen]) |
                        ((uint16_t)s_buf[p + 3 + klen] << 8);
        uint16_t total = 4 + klen + dlen;
        if (p + total > PREFS_REGION_SIZE) {
            break; /* corrupt table; treat as full */
        }
        p += total;
    }
    return PREFS_REGION_SIZE;
}

/* Drop deleted entries in place; the walk stops at the first end marker. */
void Preferences::_compact(void)
{
    uint16_t w = PREFS_NS_STORE;
    uint16_t p = PREFS_NS_STORE;

    while (p + 2 <= PREFS_REGION_SIZE) {
        uint8_t type = s_buf[p];
        uint8_t klen = s_buf[p + 1];
        if (type == 0 && klen == 0) {
            break;
        }
        uint16_t dlen = (uint16_t)(s_buf[p + 2 + klen]) |
                        ((uint16_t)s_buf[p + 3 + klen] << 8);
        uint16_t total = 4 + klen + dlen;
        if (p + total > PREFS_REGION_SIZE) {
            break;
        }
        if (type != PREFS_TYPE_DELETED) {
            if (w != p) {
                memmove(&s_buf[w], &s_buf[p], total);
            }
            w += total;
        }
        p += total;
    }
    if (w + 2 <= PREFS_REGION_SIZE) {
        s_buf[w] = 0;
        s_buf[w + 1] = 0;
        if (w + 2 < PREFS_REGION_SIZE) {
            memset(&s_buf[w + 2], 0xFF, PREFS_REGION_SIZE - w - 2);
        }
    }
}

bool Preferences::_append(const char *key, uint8_t type,
                          const void *data, uint16_t dlen)
{
    uint16_t klen = strlen(key);
    uint16_t end;

    if (klen == 0 || klen > 255) {
        return false;
    }
    _compact(); /* reclaim deleted entries for room */
    end = _tableEnd();
    if (end + 4 + klen + dlen + 2 > PREFS_REGION_SIZE) {
        return false; /* no room (incl. the end marker) */
    }
    s_buf[end] = type;
    s_buf[end + 1] = (uint8_t)klen;
    memcpy(&s_buf[end + 2], key, klen);
    s_buf[end + 2 + klen] = (uint8_t)(dlen & 0xFF);
    s_buf[end + 3 + klen] = (uint8_t)(dlen >> 8);
    if (dlen) {
        memcpy(&s_buf[end + 4 + klen], data, dlen);
    }
    s_buf[end + 4 + klen + dlen] = 0; /* end marker */
    s_buf[end + 5 + klen + dlen] = 0;
    _dirty = true;
    return true;
}

bool Preferences::_remove(const char *key)
{
    uint16_t p = PREFS_NS_STORE;
    size_t klen = strlen(key);

    while (p + 2 <= PREFS_REGION_SIZE) {
        uint8_t type = s_buf[p];
        uint8_t klen8 = s_buf[p + 1];
        if (type == 0 && klen8 == 0) {
            return false;
        }
        uint16_t dlen = (uint16_t)(s_buf[p + 2 + klen8]) |
                        ((uint16_t)s_buf[p + 3 + klen8] << 8);
        if (type != PREFS_TYPE_DELETED && klen == (size_t)klen8 &&
            memcmp(&s_buf[p + 2], key, klen) == 0) {
            s_buf[p] = PREFS_TYPE_DELETED;
            _dirty = true;
            return true;
        }
        p += 4 + klen8 + dlen;
    }
    return false;
}

bool Preferences::begin(const char *name, bool readOnly)
{
    uint32_t magic;

    _readOnly = readOnly;
    _dirty = false;
    if (strlen(name) > PREFS_NS_MAX - 1) {
        return false;
    }
    strcpy(_ns, name);

    if (!_handle) {
        bl_mtd_handle_t h = NULL;
        if (bl_mtd_open("DATA", &h, BL_MTD_OPEN_FLAG_NONE) == 0) {
            _handle = h;
        }
    }
    if (_handle) {
        bl_mtd_read(_handle, PREFS_REGION_OFFSET, PREFS_REGION_SIZE, s_buf);
    } else {
        memset(s_buf, 0xFF, PREFS_REGION_SIZE);
    }

    memcpy(&magic, s_buf, 4);
    if (magic == PREFS_MAGIC &&
        memcmp(&s_buf[4], name, strlen(name) + 1) == 0) {
        return true; /* same namespace already stored */
    }

    _reset(name); /* fresh header; commit() persists it on first end() */
    _dirty = true;
    return true;
}

void Preferences::end(void)
{
    if (_dirty) {
        commit();
    }
    if (_handle) {
        bl_mtd_close(_handle);
        _handle = NULL;
    }
    _readOnly = false;
}

bool Preferences::commit(void)
{
    if (!_handle || !_dirty) {
        return false;
    }
    _compact();
    if (bl_mtd_erase(_handle, PREFS_REGION_OFFSET, PREFS_REGION_SIZE) != 0) {
        return false;
    }
    if (bl_mtd_write(_handle, PREFS_REGION_OFFSET, PREFS_REGION_SIZE, s_buf) != 0) {
        return false;
    }
    _dirty = false;
    return true;
}

bool Preferences::isKey(const char *key)
{
    return prefs_find(key, 0, NULL, NULL);
}

bool Preferences::remove(const char *key)
{
    if (_readOnly) {
        return false;
    }
    return _remove(key);
}

bool Preferences::clear(void)
{
    if (_readOnly) {
        return false;
    }
    _reset(_ns);
    _dirty = true;
    return true;
}

size_t Preferences::freeEntries(void)
{
    uint16_t end = _tableEnd();
    return (size_t)(PREFS_REGION_SIZE - end);
}

/* ---------------- put ---------------- */

#define PREFS_PUT(name, type, ctype, expr) \
    size_t Preferences::put##name(const char *key, ctype value) \
    { \
        ctype v = (expr); \
        if (_readOnly) return 0; \
        _remove(key); \
        return _append(key, type, &v, sizeof v) ? sizeof v : 0; \
    }

PREFS_PUT(Char,   PREFS_TYPE_CHAR,   int8_t,  value)
PREFS_PUT(UChar,  PREFS_TYPE_UCHAR,  uint8_t, value)
PREFS_PUT(Short,  PREFS_TYPE_SHORT,  int16_t, value)
PREFS_PUT(UShort, PREFS_TYPE_USHORT, uint16_t, value)
PREFS_PUT(Int,    PREFS_TYPE_INT,    int32_t, value)
PREFS_PUT(UInt,   PREFS_TYPE_UINT,   uint32_t, value)
PREFS_PUT(Long,   PREFS_TYPE_INT,    int32_t, value)
PREFS_PUT(ULong,  PREFS_TYPE_UINT,   uint32_t, value)
PREFS_PUT(Long64, PREFS_TYPE_INT64,  int64_t, value)
PREFS_PUT(ULong64, PREFS_TYPE_UINT64, uint64_t, value)
PREFS_PUT(Float,  PREFS_TYPE_FLOAT,  float,   value)
PREFS_PUT(Double, PREFS_TYPE_DOUBLE, double,  value)
PREFS_PUT(Bool,   PREFS_TYPE_BOOL,   bool,    value ? 1 : 0)

size_t Preferences::putString(const char *key, const char *value)
{
    size_t len;

    if (_readOnly) {
        return 0;
    }
    len = strlen(value);
    if (len > 0xFFFF) {
        return 0;
    }
    _remove(key);
    return _append(key, PREFS_TYPE_STRING, value, (uint16_t)len) ? len : 0;
}

size_t Preferences::putString(const char *key, const String &value)
{
    return putString(key, value.c_str());
}

size_t Preferences::putBytes(const char *key, const void *value, size_t len)
{
    if (_readOnly) {
        return 0;
    }
    if (len > 0xFFFF) {
        return 0;
    }
    _remove(key);
    return _append(key, PREFS_TYPE_BYTES, value, (uint16_t)len) ? len : 0;
}

/* ---------------- get ---------------- */

#define PREFS_GET(name, type, ctype) \
    ctype Preferences::get##name(const char *key, ctype defaultValue) \
    { \
        uint16_t dlen; \
        const uint8_t *d; \
        if (!prefs_find(key, type, &dlen, &d) || dlen < sizeof(ctype)) { \
            return defaultValue; \
        } \
        ctype v; \
        memcpy(&v, d, sizeof v); \
        return v; \
    }

PREFS_GET(Char,   PREFS_TYPE_CHAR,   int8_t)
PREFS_GET(UChar,  PREFS_TYPE_UCHAR,  uint8_t)
PREFS_GET(Short,  PREFS_TYPE_SHORT,  int16_t)
PREFS_GET(UShort, PREFS_TYPE_USHORT, uint16_t)
PREFS_GET(Int,    PREFS_TYPE_INT,    int32_t)
PREFS_GET(UInt,   PREFS_TYPE_UINT,   uint32_t)
PREFS_GET(Long,   PREFS_TYPE_INT,    int32_t)
PREFS_GET(ULong,  PREFS_TYPE_UINT,   uint32_t)
PREFS_GET(Long64, PREFS_TYPE_INT64,  int64_t)
PREFS_GET(ULong64, PREFS_TYPE_UINT64, uint64_t)
PREFS_GET(Float,  PREFS_TYPE_FLOAT,  float)
PREFS_GET(Double, PREFS_TYPE_DOUBLE, double)
PREFS_GET(Bool,   PREFS_TYPE_BOOL,   bool)

String Preferences::getString(const char *key, const String &defaultValue)
{
    uint16_t dlen;
    const uint8_t *d;

    /* STRING entries normally; BYTES entries accepted too (raw payload). */
    if (prefs_find(key, PREFS_TYPE_STRING, &dlen, &d)) {
        return String((const char *)d, dlen);
    }
    if (prefs_find(key, PREFS_TYPE_BYTES, &dlen, &d)) {
        return String((const char *)d, dlen);
    }
    return defaultValue;
}

String Preferences::getString(const char *key, const char *defaultValue)
{
    uint16_t dlen;
    const uint8_t *d;

    if (prefs_find(key, PREFS_TYPE_STRING, &dlen, &d)) {
        return String((const char *)d, dlen);
    }
    if (prefs_find(key, PREFS_TYPE_BYTES, &dlen, &d)) {
        return String((const char *)d, dlen);
    }
    return String(defaultValue);
}

size_t Preferences::getBytes(const char *key, void *buf, size_t maxLen)
{
    uint16_t dlen;
    const uint8_t *d;
    size_t n;

    if (!prefs_find(key, PREFS_TYPE_BYTES, &dlen, &d)) {
        return 0;
    }
    n = (dlen < maxLen) ? dlen : maxLen;
    if (n) {
        memcpy(buf, d, n);
    }
    return n;
}

size_t Preferences::getBytesLength(const char *key)
{
    uint16_t dlen;

    if (prefs_find(key, PREFS_TYPE_BYTES, &dlen, NULL)) {
        return dlen;
    }
    return 0;
}
