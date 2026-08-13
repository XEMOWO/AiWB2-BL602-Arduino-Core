/*
 * Udp.h — abstract UDP base class (ESP8266-compatible).
 *
 * Concrete implementation (WiFiUDP) derives from this. Kept 1:1 with the
 * ESP8266 core so packet-based libraries compile unchanged.
 *
 * NOTE on rawIPAddress: on BL602 this returns the raw network-order byte
 * array, which lwIP socket code can pass to sendto().
 *
 * Mirror of cores/esp8266/Udp.h in the esp8266/Arduino repo.
 */
#ifndef udp_h
#define udp_h

#include <Stream.h>
#include <IPAddress.h>

class UDP : public Stream
{
public:
    virtual ~UDP() {}

    /* initialize, start listening on specified port. 1 if successful, 0 if no sockets available */
    virtual uint8_t begin(uint16_t) = 0;
    /* initialize, start listening on specified multicast IP address and port. 1 if ok, 0 on failure */
    virtual uint8_t beginMulticast(IPAddress, uint16_t) { return 0; }
    virtual void stop() = 0;  /* finish with the UDP socket */

    /* ---- sending ---- */
    virtual int beginPacket(IPAddress ip, uint16_t port) = 0;
    virtual int beginPacket(const char *host, uint16_t port) = 0;
    virtual int endPacket() = 0;               /* 1 if sent, 0 on error */
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;

    /* ---- receiving ---- */
    virtual int parsePacket() = 0;             /* size of next packet, 0 if none */
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int read(unsigned char *buffer, size_t len) = 0;
    virtual int read(char *buffer, size_t len) = 0;
    virtual int peek() = 0;
    virtual void flush() = 0;                  /* finish reading current packet */

    virtual IPAddress remoteIP() = 0;
    virtual uint16_t remotePort() = 0;

protected:
    uint8_t *rawIPAddress(IPAddress &addr)       { return addr.raw_address(); }
    const uint8_t *rawIPAddress(const IPAddress &addr) { return addr.raw_address(); }
};

#endif /* udp_h */
