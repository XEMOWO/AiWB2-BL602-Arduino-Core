/*
 * IPAddress.h — standard Arduino IPAddress value type (ESP32-compatible).
 *
 * Holds a 32-bit IPv4 address in network byte order and prints as dotted
 * quad. Used by WiFi (localIP etc) and any future network clients.
 */
#ifndef IPAddress_h
#define IPAddress_h

#include <Arduino.h>
#include <stdint.h>
#include <Print.h>

class IPAddress : public Printable
{
public:
    /* Empty (0.0.0.0) */
    IPAddress() { _addr.dword = 0; }

    IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth)
    {
        _addr.bytes[0] = first;
        _addr.bytes[1] = second;
        _addr.bytes[2] = third;
        _addr.bytes[3] = fourth;
    }

    /* From a host-order uint32 (0x7F000001 == 127.0.0.1) */
    IPAddress(uint32_t address) { _addr.dword = address; }

    IPAddress(const uint8_t *address)
    {
        _addr.bytes[0] = address[0];
        _addr.bytes[1] = address[1];
        _addr.bytes[2] = address[2];
        _addr.bytes[3] = address[3];
    }

    bool operator==(const IPAddress &other) const { return _addr.dword == other._addr.dword; }
    bool operator!=(const IPAddress &other) const { return _addr.dword != other._addr.dword; }
    bool operator==(uint32_t address) const { return _addr.dword == address; }

    /* host-order uint32 access */
    uint32_t operator()(void) const { return _addr.dword; }

    uint8_t operator[](int index) const { return _addr.bytes[index]; }
    uint8_t &operator[](int index) { return _addr.bytes[index]; }

    /* from a host-order uint32 */
    IPAddress &operator=(uint32_t address)
    {
        _addr.dword = address;
        return *this;
    }

    size_t printTo(Print &p) const
    {
        size_t n = 0;
        for (int i = 0; i < 4; i++) {
            n += p.print(_addr.bytes[i], DEC);
            if (i < 3) n += p.print('.');
        }
        return n;
    }

    /* dotted-quad string form, e.g. "192.168.1.10" */
    String toString(void) const
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                 (unsigned)_addr.bytes[0], (unsigned)_addr.bytes[1],
                 (unsigned)_addr.bytes[2], (unsigned)_addr.bytes[3]);
        return String(buf);
    }

    operator String(void) const { return toString(); }

    /* host-order uint32 with byte order preserved the way WiFi APIs want it */
    uint32_t asUint32(void) const { return _addr.dword; }

private:
    union {
        uint8_t  bytes[4];  /* big-endian [a.b.c.d] */
        uint32_t dword;     /* network-order bit pattern */
    } _addr;
};

#endif /* IPAddress_h */
