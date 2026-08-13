/*
    w5500.h — Wiznet W5500 raw-MAC driver stub for the Ai-WB2-12F (BL602).

    The ESP8266 core's LwipIntfDev<RawDev> template drives the raw Ethernet
    device through begin()/end()/isLinked()/isLinkDetectable() and (when
    polling) the protected frame hooks. The WB2 has no SPI Ethernet hardware,
    so every method is a no-op and begin() reports "hardware not responding".
    The class shape mirrors the reference utility/w5500.h so LwipIntfDev and
    any sketch that touches the raw device compile unchanged.
*/

#ifndef W5500_H
#define W5500_H

#include <stdint.h>
#include <Arduino.h>
#include <SPI.h>

class Wiznet5500
{
public:
    Wiznet5500(int8_t cs = SS, SPIClass& spi = SPI, int8_t intr = -1)
    {
        (void)cs; (void)spi; (void)intr;
    }

    // No W5500 on WB2: report failure so LwipIntfDev::begin() is honest.
    boolean begin(const uint8_t* address)
    {
        (void)address;
        return false;
    }

    void end() {}

    bool isLinked()
    {
        return false;
    }

    constexpr bool isLinkDetectable() const
    {
        return false;
    }

protected:
    // frame hooks used by LwipIntfDev::handlePackets() (never started on WB2)
    uint16_t readFrameSize()
    {
        return 0;
    }
    void discardFrame(uint16_t framesize)
    {
        (void)framesize;
    }
    uint16_t readFrameData(uint8_t* buffer, uint16_t bufsize)
    {
        (void)buffer; (void)bufsize;
        return 0;
    }
    uint16_t sendFrame(const uint8_t* data, uint16_t datalen)
    {
        (void)data; (void)datalen;
        return 0;
    }
};

#endif  // W5500_H
