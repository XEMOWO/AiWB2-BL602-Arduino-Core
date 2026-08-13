/*
 * Preferences.h — ESP32-compatible key-value storage for the Ai-WB2-12F.
 *
 * The API mirrors arduino-esp32's Preferences (NVS), but storage is a simple
 * serialized table in its own 4K sector of the DATA flash partition. Like the
 * EEPROM library, writes update a RAM shadow and commit() (or end()) persists
 * them — flash writes need an erase-first sector, so buffer + commit.
 *
 *   Preferences prefs;
 *   prefs.begin("cfg", false);          // readOnly = true forbids writes
 *   prefs.putInt("count", 42);
 *   prefs.putString("name", "wb2");
 *   prefs.end();                        // or commit() to persist now
 *
 * Only one namespace may be open at a time (a single table per instance).
 */
#ifndef Preferences_h
#define Preferences_h

#include <stdint.h>
#include <stddef.h>
#include <Arduino.h> /* String */

class Preferences
{
public:
    Preferences();

    /* Loads (or creates) the named namespace. readOnly forbids put/remove/clear. */
    bool begin(const char *name, bool readOnly = false);
    void end(void);      /* commits pending writes, then closes flash */

    bool commit(void);   /* erase + write the RAM shadow to flash */

    bool isKey(const char *key); /* true if key exists, any type */
    bool remove(const char *key);
    bool clear(void);            /* empty the whole namespace */

    size_t freeEntries(void);    /* free bytes left in the region */

    /* ---- put (returns bytes stored, 0 on failure / read-only) ---- */
    size_t putChar(const char *key, int8_t value);
    size_t putUChar(const char *key, uint8_t value);
    size_t putShort(const char *key, int16_t value);
    size_t putUShort(const char *key, uint16_t value);
    size_t putInt(const char *key, int32_t value);
    size_t putUInt(const char *key, uint32_t value);
    size_t putLong(const char *key, int32_t value);
    size_t putULong(const char *key, uint32_t value);
    size_t putLong64(const char *key, int64_t value);
    size_t putULong64(const char *key, uint64_t value);
    size_t putFloat(const char *key, float value);
    size_t putDouble(const char *key, double value);
    size_t putBool(const char *key, bool value);
    size_t putString(const char *key, const char *value);
    size_t putString(const char *key, const String &value);
    size_t putBytes(const char *key, const void *value, size_t len);

    /* ---- get (returns defaultValue when absent or type mismatch) ---- */
    int8_t   getChar(const char *key, int8_t defaultValue = 0);
    uint8_t  getUChar(const char *key, uint8_t defaultValue = 0);
    int16_t  getShort(const char *key, int16_t defaultValue = 0);
    uint16_t getUShort(const char *key, uint16_t defaultValue = 0);
    int32_t  getInt(const char *key, int32_t defaultValue = 0);
    uint32_t getUInt(const char *key, uint32_t defaultValue = 0);
    int32_t  getLong(const char *key, int32_t defaultValue = 0);
    uint32_t getULong(const char *key, uint32_t defaultValue = 0);
    int64_t  getLong64(const char *key, int64_t defaultValue = 0);
    uint64_t getULong64(const char *key, uint64_t defaultValue = 0);
    float    getFloat(const char *key, float defaultValue = 0.0f);
    double   getDouble(const char *key, double defaultValue = 0.0);
    bool     getBool(const char *key, bool defaultValue = false);
    String   getString(const char *key, const String &defaultValue = String());
    String   getString(const char *key, const char *defaultValue = "");
    size_t   getBytes(const char *key, void *buf, size_t maxLen); /* returns bytes copied */
    size_t   getBytesLength(const char *key);

private:
    enum {
        PREFS_TYPE_BOOL    = 1,
        PREFS_TYPE_CHAR    = 2,
        PREFS_TYPE_UCHAR   = 3,
        PREFS_TYPE_SHORT   = 4,
        PREFS_TYPE_USHORT  = 5,
        PREFS_TYPE_INT     = 6,
        PREFS_TYPE_UINT    = 7,
        PREFS_TYPE_FLOAT   = 8,
        PREFS_TYPE_DOUBLE  = 9,
        PREFS_TYPE_STRING  = 10,
        PREFS_TYPE_BYTES   = 11,
        PREFS_TYPE_INT64   = 12,
        PREFS_TYPE_UINT64  = 13,
        PREFS_TYPE_DELETED = 0xFF
    };

    void    *_handle;                 /* bl_mtd_handle_t, opaque here */
    bool     _readOnly;
    bool     _dirty;
    char     _ns[32];

    void     _reset(const char *name);
    uint16_t _tableEnd(void);
    void     _compact(void);
    bool     _append(const char *key, uint8_t type, const void *data, uint16_t dlen);
    bool     _remove(const char *key);
};

#endif /* Preferences_h */
