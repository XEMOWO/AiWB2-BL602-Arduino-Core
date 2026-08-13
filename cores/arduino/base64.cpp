/*
 * base64.cpp — base64 encoding, self-contained.
 *
 * Implements the ESP8266 base64 class without the vendored libb64 backend.
 * The alphabet and padding follow RFC 4648. `doNewLines` wraps the output at
 * 72 columns with '\n' (matching libb64's default line size), which the
 * ESP8266 API documents.
 */
#include "base64.h"

static const char s_b64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char s_b64_char(uint8_t v)
{
    return s_b64_alphabet[v & 0x3F];
}

String base64::encode(const uint8_t *data, size_t length, bool doNewLines)
{
    String out;
    size_t i = 0;
    size_t col = 0;

    out.reserve(((length + 2) / 3) * 4 + (doNewLines ? (length / 54) + 2 : 0));

    while (i + 3 <= length) {
        uint32_t triple = ((uint32_t)data[i] << 16) |
                          ((uint32_t)data[i + 1] << 8) |
                          (uint32_t)data[i + 2];
        out += s_b64_char(triple >> 18);
        out += s_b64_char(triple >> 12);
        out += s_b64_char(triple >> 6);
        out += s_b64_char(triple);
        i += 3;
        col += 4;
        if (doNewLines && col >= 72 && i < length) {
            out += '\n';
            col = 0;
        }
    }

    /* trailing 1 or 2 bytes with padding */
    if (i < length) {
        uint32_t tail = (uint32_t)data[i] << 16;
        if (i + 1 < length) {
            tail |= (uint32_t)data[i + 1] << 8;
        }
        out += s_b64_char(tail >> 18);
        out += s_b64_char(tail >> 12);
        out += (i + 1 < length) ? s_b64_char(tail >> 6) : '=';
        out += '=';
    }

    return out;
}
