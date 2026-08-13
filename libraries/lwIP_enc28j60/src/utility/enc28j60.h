/*
    enc28j60.h — Microchip ENC28J60 raw-MAC driver stub for the Ai-WB2-12F (BL602).

    Same contract as lwIP_w5500/src/utility/w5500.h: the WB2 has no SPI
    Ethernet hardware, so the raw device is a no-op and begin() reports
    "hardware not responding".
*/

#ifndef ENC28J60_H
#define ENC28J60_H

#include <stdint.h>
#include <Arduino.h>
#include <SPI.h>

class ENC28J60
{
public:
    ENC28J60(int8_t cs = SS, SPIClass& spi = SPI, int8_t intr = -1)
    {
        (void)cs; (void)spi; (void)intr;
    }

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

#endif  // ENC28J60_H
