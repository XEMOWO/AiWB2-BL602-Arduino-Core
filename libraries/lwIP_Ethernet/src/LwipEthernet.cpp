/*
    LwipEthernet.cpp — SPI4EthInit() for the Ai-WB2-12F (BL602).

    The ESP8266 implementation puts the shared SPI bus into Ethernet mode
    (WIZSPI clock divider, mode 0). On the WB2 there is no SPI Ethernet
    hardware and the SPI library only drives SCK/MOSI/MISO (GPIO22 is reserved
    for flash XIP and must never be touched), so this hook is deliberately
    empty — the classic "Arduino Ethernet self-initializes SPI" contract is
    kept for API compatibility.
*/

#include <LwipEthernet.h>

void SPI4EthInit()
{
    // no SPI Ethernet hardware on WB2; nothing to configure
}
