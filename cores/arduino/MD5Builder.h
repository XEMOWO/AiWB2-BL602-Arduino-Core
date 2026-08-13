/*
 * MD5Builder.h — incremental MD5 builder (ESP8266-compatible).
 *
 * Same API as cores/esp8266/MD5Builder.h in the esp8266/Arduino repo, backed
 * by our self-contained md5 implementation.
 */
#ifndef __ESP8266_MD5_BUILDER__
#define __ESP8266_MD5_BUILDER__

#include <WString.h>
#include <Stream.h>
#include "md5.h"

class MD5Builder {
    private:
        md5_context_t _ctx;
        uint8_t _buf[16];
    public:
        void begin(void);
        void add(const uint8_t *data, const uint16_t len);
        void add(const char *data) { add((const uint8_t *)data, strlen(data)); }
        void add(char *data)       { add((const char *)data); }
        void add(const String &data) { add(data.c_str()); }
        void addHexString(const char *data);
        void addHexString(char *data) { addHexString((const char *)data); }
        void addHexString(const String &data) { addHexString(data.c_str()); }
        bool addStream(Stream &stream, const size_t maxLen);
        void calculate(void);
        void getBytes(uint8_t *output) const;
        void getChars(char *output) const;
        String toString(void) const;
};

#endif /* __ESP8266_MD5_BUILDER__ */
