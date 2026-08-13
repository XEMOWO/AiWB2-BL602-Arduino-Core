/*
 * lwip_socket_util.h — small shared helpers for the socket-based
 * WiFiClient / WiFiServer / WiFiUDP backends.
 *
 * INCLUDE ORDER: like every file in this library, the SDK + lwIP headers must
 * come before Arduino.h (FreeRTOS's platform.h defines a dead `GPIO_REG` macro
 * that wiring_private.h inside Arduino.h #undef's and re-defines). The .cpp
 * files include WB2WiFiCommon.h first, so all of this is normally a no-op.
 */
#ifndef WB2_LWIP_SOCKET_UTIL_H
#define WB2_LWIP_SOCKET_UTIL_H

#include <string.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <lwip/def.h>

#include "lwip_compat_undef.h"

#include <Arduino.h>
#include "IPAddress.h"

/* Build a sockaddr_in from an IPAddress + port.
 *
 * The core's IPAddress holds the network-order bit pattern in `dword`: for
 * 192.168.4.1 the memory bytes are {192,168,4,1}, which is exactly what lwIP
 * stores in sin_addr.s_addr (little-endian BL602). So asUint32() is what
 * lwIP's in_addr wants, and reading sin_addr.s_addr back into IPAddress()
 * yields the same dotted quad. */
static inline struct sockaddr_in s_wb2_sockaddr(IPAddress ip, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip.asUint32();
    return addr;
}

/* Resolve a hostname with the lwIP DNS client (blocking, like the reference
 * hostByName). Returns false on failure. */
static inline bool s_wb2_resolve(const char* host, IPAddress* out)
{
    struct hostent* he = lwip_gethostbyname(host);
    if (he && he->h_addr_list[0]) {
        *out = IPAddress((const uint8_t*)he->h_addr_list[0]);
        return true;
    }
    return false;
}

/* Put a socket in non-blocking mode. lwIP's ioctl only supports FIONBIO and
 * FIONREAD, so this is the only way to get non-blocking behaviour. */
static inline void s_wb2_set_nonblock(int fd)
{
    int on = 1;
    lwip_ioctl(fd, FIONBIO, &on);
}

/* select() with a timeout on a single socket. writable=true waits for send
 * space, false for readable. Returns >0 if ready, 0 on timeout, <0 on error. */
static inline int s_wb2_wait_socket(int fd, bool writable, uint32_t timeoutMs)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return lwip_select(fd + 1,
                       writable ? NULL : &set,
                       writable ? &set : NULL,
                       NULL, &tv);
}

#endif /* WB2_LWIP_SOCKET_UTIL_H */
