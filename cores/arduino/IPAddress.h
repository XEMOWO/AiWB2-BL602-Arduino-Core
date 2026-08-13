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
/* lwIP address types. The ESP8266 IPAddress is a decorator over lwIP's
 * ip_addr_t, and third-party code (AddrList, UdpContext, WiFi internals)
 * converts freely between the two — mirror that here. */
#include <lwip/ip_addr.h>
#include <lwip/ip4_addr.h>

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

    /* ESP8266-style truthiness: non-zero address. */
    operator bool() const { return _addr.dword != 0; }

    /* ESP8266-style implicit u32 view (network-order bit pattern). LEAmDNS
     * does `ip_info.ip.addr = udpContext->getRemoteAddress();`, and other
     * libs pass IPAddress where a u32 is expected. */
    operator uint32_t() const { return _addr.dword; }
    operator uint32_t()       { return _addr.dword; }

    /* ESP8266-compatible classification. BL602's IPAddress holds an IPv4
     * address only, so isV4() is always true / isV6() always false. */
    bool isV4() const { return true; }
    bool isV6() const { return false; }
    bool isSet() const { return _addr.dword != 0; }
    void clear() { _addr.dword = 0; }

    /* IPv4 link-local 169.254.0.0/16 — the subset of ESP8266's
     * ip_addr_islinklocal() an IPv4-only address can hit. */
    bool isLocal() const
    {
        return _addr.bytes[0] == 169 && _addr.bytes[1] == 254;
    }

    /* Parse a dotted-quad IPv4 string (ESP8266 API — NAPTCaptivePortal's
     * `hAddr.fromString(server.hostHeader())`). Returns false on any
     * non-numeric / out-of-range / malformed input. BL602 is IPv4-only, so
     * fromString6 never succeeds and fromString just tries IPv4. */
    bool fromString(const char *address);
    bool fromString(const String &address) { return fromString(address.c_str()); }
    bool fromString4(const char *address);

    /* Convert to/from lwIP ip_addr_t (IPv4 only). The ESP8266 IPAddress is
     * built on ip_addr_t; netif code (AddrList) hands us these. Only the
     * ip_addr_t form is provided: the SDK's lwIP is IPv4-only (its LWIP_IPV6
     * is gated behind CFG_IPV6, off by default), so ip_addr_t IS ip4_addr_t
     * and overloads for both would collide. */
    IPAddress(const ip_addr_t& lwip_addr) { _addr.dword = ip_2_ip4(&lwip_addr)->addr; }
    IPAddress(const ip_addr_t* lwip_addr) { _addr.dword = ip_2_ip4(lwip_addr)->addr; }
    IPAddress& operator=(const ip_addr_t& lwip_addr) { _addr.dword = ip_2_ip4(&lwip_addr)->addr; return *this; }
    IPAddress& operator=(const ip_addr_t* lwip_addr) { _addr.dword = ip_2_ip4(lwip_addr)->addr; return *this; }

    operator ip_addr_t() const
    {
        ip_addr_t a;
        ip_addr_set_ip4_u32_val(a, _addr.dword);
        return a;
    }

    /* ESP8266 exposes lwIP pointer conversions (its _ip member IS an
     * ip_addr_t). BL602 stores the address in a union, so these hand back a
     * pointer to a shared snapshot. That only ever serves transient lwIP
     * calls (udp_bind/udp_connect & co copy the address out immediately),
     * which is exactly how the ESP8266 ecosystem uses them. */
    operator ip_addr_t*()              { return &lwipSnapshot(); }
    operator const ip_addr_t*() const  { return &lwipSnapshot(); }

    /* host-order uint32 access */
    uint32_t operator()(void) const { return _addr.dword; }

    /* ESP8266-style lwIP IPv4 view. On the reference core this returns the
     * ip4_addr_t u32 (network byte order); our _addr.dword holds the same bit
     * pattern, so this is an exact alias — used by LwipIntf::ipAddressReorder
     * and third-party netif code. */
    uint32_t &v4() { return _addr.dword; }
    const uint32_t &v4() const { return _addr.dword; }

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

    /* Raw 4-byte array (network order). ESP8266/AVR style accessor used by the
     * Client/UDP base classes (rawIPAddress()) and socket code. */
    uint8_t *raw_address(void)             { return _addr.bytes; }
    const uint8_t *raw_address(void) const { return _addr.bytes; }

    /* ESP8266 raw IPv6 view. BL602's IPAddress is IPv4-only (no LWIP_IPV6), so
     * this matches the reference core's !LWIP_IPV6 branch and returns null —
     * Netdump and other libs guard on it. */
    uint16_t *raw6()             { return nullptr; }
    const uint16_t *raw6() const { return nullptr; }

private:
    /* Backing store for the ip_addr_t* conversions above (see the comment on
     * operator const ip_addr_t*()). */
    ip_addr_t &lwipSnapshot() const
    {
        static ip_addr_t snap;
        ip_addr_set_ip4_u32_val(snap, _addr.dword);
        return snap;
    }

    union {
        uint8_t  bytes[4];  /* big-endian [a.b.c.d] */
        uint32_t dword;     /* network-order bit pattern */
    } _addr;
};

/* printf() helpers for a netif id, as in the ESP8266 core's IPAddress.h.
 * Expanded at the use site, so <lwip/netif.h> (source of netif_get_index)
 * only needs to be visible there — LEAmDNS and friends include it already. */
#define NETIFID_STR        "%c%c%u"
#define NETIFID_VAL(netif) \
        ((netif)? (netif)->name[0]: '-'),     \
        ((netif)? (netif)->name[1]: '-'),     \
        ((netif)? netif_get_index(netif): 42)

/* ESP8266/ESP32-style address constants. Defined in IPAddress.cpp. The lwIP
 * headers alias these names to IPADDR_* macros; the WiFi library #undef's them
 * (lwip_compat_undef.h) so these object constants win, as on the reference. */
extern const IPAddress INADDR_ANY;        /* 0.0.0.0 */
extern const IPAddress INADDR_NONE;       /* 255.255.255.255 */
extern const IPAddress INADDR_LOOPBACK;   /* 127.0.0.1 */
extern const IPAddress INADDR_BROADCAST;  /* 255.255.255.255 */

#endif /* IPAddress_h */
