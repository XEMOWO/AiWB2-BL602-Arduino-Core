/*
 * base64.h — base64 encoding (ESP8266-compatible).
 *
 * Same API as cores/esp8266/base64.h in the esp8266/Arduino repo. The
 * implementation is self-contained (no vendored libb64) so it links on any
 * toolchain build.
 */
#ifndef CORE_BASE64_H_
#define CORE_BASE64_H_

#include <WString.h>

class base64
{
public:
    // doNewLines inserts a newline every 72 encoded characters, matching the
    // libb64 backend the ESP8266 core uses. Pass false for URIs/JSON.
    static String encode(const uint8_t *data, size_t length, bool doNewLines);
    static inline String encode(const String &text, bool doNewLines)
    {
        return encode((const uint8_t *)text.c_str(), text.length(), doNewLines);
    }

    // esp32 compat (no newlines):
    static inline String encode(const uint8_t *data, size_t length)
    {
        return encode(data, length, false);
    }
    static inline String encode(const String &text)
    {
        return encode(text, false);
    }
};

#endif /* CORE_BASE64_H_ */
