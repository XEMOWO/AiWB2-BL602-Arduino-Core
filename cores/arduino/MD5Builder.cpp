/*
 * MD5Builder.cpp — incremental MD5 builder (ESP8266-compatible).
 *
 * Ported from cores/esp8266/MD5Builder.cpp (esp8266/Arduino repo,
 * Copyright (c) 2015 Hristo Gochkov, LGPL 2.1+). std::unique_ptr for the
 * hex-decoding scratch is fine on freestanding libstdc++ (allocation only).
 */
#include <Arduino.h>
#include <MD5Builder.h>
#include <memory>

static uint8_t hex_char_to_byte(uint8_t c)
{
    return (c >= 'a' && c <= 'f') ? (c - ((uint8_t)'a' - 0xa)) :
           (c >= 'A' && c <= 'F') ? (c - ((uint8_t)'A' - 0xA)) :
           (c >= '0' && c <= '9') ? (c - (uint8_t)'0') : 0;
}

void MD5Builder::begin(void)
{
    memset(_buf, 0x00, 16);
    MD5Init(&_ctx);
}

void MD5Builder::add(const uint8_t *data, const uint16_t len)
{
    MD5Update(&_ctx, data, len);
}

void MD5Builder::addHexString(const char *data)
{
    uint16_t i, len = strlen(data);
    std::unique_ptr<uint8_t[]> tmp{new (std::nothrow) uint8_t[len / 2]};

    if (!tmp) {
        return;
    }

    for (i = 0; i < len; i += 2) {
        uint8_t high = hex_char_to_byte(data[i]);
        uint8_t low = hex_char_to_byte(data[i + 1]);
        tmp[i / 2] = (high << 4) | low;
    }
    add(tmp.get(), len / 2);
}

bool MD5Builder::addStream(Stream &stream, const size_t maxLen)
{
    const int buf_size = 512;
    int max = maxLen / buf_size;
    int bytesLeft = maxLen % buf_size;
    std::unique_ptr<uint8_t[]> buf{new (std::nothrow) uint8_t[buf_size]};

    if (!buf) {
        return false;
    }

    while (max--) {
        int numBytesRead = stream.readBytes((char *)buf.get(), buf_size);
        if (numBytesRead != buf_size) {
            return false;
        }
        MD5Update(&_ctx, buf.get(), numBytesRead);
    }
    if (bytesLeft) {
        int numBytesRead = stream.readBytes((char *)buf.get(), bytesLeft);
        if (numBytesRead != bytesLeft) {
            return false;
        }
        MD5Update(&_ctx, buf.get(), numBytesRead);
    }
    return true;
}

void MD5Builder::calculate(void)
{
    MD5Final(_buf, &_ctx);
}

void MD5Builder::getBytes(uint8_t *output) const
{
    memcpy(output, _buf, 16);
}

void MD5Builder::getChars(char *output) const
{
    const char *hexdigits = "0123456789abcdef";
    for (int i = 0; i < 16; i++) {
        output[i * 2]     = hexdigits[_buf[i] >> 4];
        output[i * 2 + 1] = hexdigits[_buf[i] & 0xF];
    }
    output[32] = 0;
}

String MD5Builder::toString(void) const
{
    char out[33];
    getChars(out);
    return String(out);
}
