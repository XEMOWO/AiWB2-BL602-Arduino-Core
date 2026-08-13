/*
 * IPAddress.cpp — ESP8266-style IP address constants (declared in IPAddress.h).
 *
 * On the reference ESP8266 core these live as `const IPAddress` globals. The
 * SDK's lwIP headers alias the same names to IPADDR_* macros, but the WiFi
 * library #undef's them (lwip_compat_undef.h) so the object constants win and
 * ESP8266 code like `IPAddress dns = INADDR_ANY;` compiles unchanged.
 */
#include "IPAddress.h"

const IPAddress INADDR_ANY;          /* 0.0.0.0 */
const IPAddress INADDR_NONE(255, 255, 255, 255);
const IPAddress INADDR_LOOPBACK(127, 0, 0, 1);
const IPAddress INADDR_BROADCAST(255, 255, 255, 255);

bool IPAddress::fromString4(const char *address)
{
    if (!address) {
        return false;
    }
    uint16_t acc = 0;
    int dots = 0;
    const char *p = address;
    while (*p) {
        char c = *p++;
        if (c >= '0' && c <= '9') {
            acc = (uint16_t)(acc * 10 + (c - '0'));
            if (acc > 255) {
                return false;
            }
        } else if (c == '.') {
            if (dots == 3) {
                return false;
            }
            _addr.bytes[dots++] = (uint8_t)acc;
            acc = 0;
        } else {
            return false;
        }
    }
    if (dots != 3) {
        return false;
    }
    _addr.bytes[3] = (uint8_t)acc;
    return true;
}

bool IPAddress::fromString(const char *address)
{
    /* BL602 is IPv4-only: the ESP8266 API tries IPv4 first, then IPv6. */
    return fromString4(address);
}
