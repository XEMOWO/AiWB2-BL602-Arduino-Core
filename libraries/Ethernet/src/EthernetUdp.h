/*
 * EthernetUdp.h - classic Arduino EthernetUDP for the Ai-WB2-12F (BL602).
 *
 * Compile-compatible shim over WiFiUDP (see Ethernet.h). Exposes the exact
 * classic EthernetUDP surface (beginPacket/parsePacket/remoteIP/...) plus the
 * classic UDP_TX_PACKET_MAX_SIZE buffer constant.
 */

#ifndef __ETHERNET_UDP_H__
#define __ETHERNET_UDP_H__

#include <Arduino.h>
#include <Udp.h>
#include <WiFiUdp.h>

/* classic Arduino EthernetUdp.h defines this for its RX buffer; some sketches
 * size a stack buffer with it */
#ifndef UDP_TX_PACKET_MAX_SIZE
#define UDP_TX_PACKET_MAX_SIZE 24
#endif

class EthernetUDP : public UDP
{
public:
    EthernetUDP() {}

    uint8_t begin(uint16_t port) override { return _udp.begin(port); }
    void stop() override { _udp.stop(); }

    uint8_t beginMulticast(IPAddress multicast, uint16_t port) override
    {
        return _udp.beginMulticast(multicast, port);
    }

    int beginPacket(IPAddress ip, uint16_t port) override { return _udp.beginPacket(ip, port); }
    int beginPacket(const char *host, uint16_t port) override { return _udp.beginPacket(host, port); }
    int endPacket() override { return _udp.endPacket(); }

    size_t write(uint8_t b) override { return _udp.write(b); }
    size_t write(const uint8_t *buffer, size_t size) override { return _udp.write(buffer, size); }
    size_t write(const char *str) { return _udp.write((const uint8_t *)str, strlen(str)); }
    using Print::write;

    int parsePacket() override { return _udp.parsePacket(); }
    int available() override { return _udp.available(); }
    int read() override { return _udp.read(); }
    int read(unsigned char *buffer, size_t len) override { return _udp.read(buffer, len); }
    int read(char *buffer, size_t len) override { return _udp.read((unsigned char *)buffer, len); }
    int peek() override { return _udp.peek(); }
    void flush() override { _udp.flush(); }

    IPAddress remoteIP() override { return _udp.remoteIP(); }
    uint16_t remotePort() override { return _udp.remotePort(); }
    IPAddress destinationIP() const { return _udp.destinationIP(); }
    uint16_t localPort() const { return _udp.localPort(); }

    operator bool() const { return (bool)_udp; }

private:
    WiFiUDP _udp;
};

#endif /* __ETHERNET_UDP_H__ */
