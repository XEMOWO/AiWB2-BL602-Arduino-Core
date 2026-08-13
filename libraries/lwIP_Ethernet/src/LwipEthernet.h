
/*
    LwipEthernet.h — Ethernet-driver helper for the Ai-WB2-12F (BL602).

    Compile-compatible port of the ESP8266 helper. ethInitDHCP() /
    ethInitStatic() run the same begin/config dance; because the WB2 has no
    SPI Ethernet hardware (LwipIntfDev is a no-op), eth.begin() reports
    "hardware not responding" and both helpers return false — the examples
    print "no hardware found" and idle, exactly the expected no-op behavior.
*/

#ifndef __LWIPETHERNET_H
#define __LWIPETHERNET_H

#include <ESP8266WiFi.h>
#include <IPAddress.h>

#include <W5100lwIP.h>
#include <W5500lwIP.h>
#include <ENC28J60lwIP.h>

// one of the lwIP interface objects is declared in the main sketch:
//   Wiznet5500lwIP eth(CSPIN);
//   Wiznet5100lwIP eth(CSPIN);
//   ENC28J60lwIP eth(CSPIN);

void SPI4EthInit();

template<class EthImpl>
bool ethInitDHCP(EthImpl& eth)
{
    SPI4EthInit();

    if (!eth.begin())
    {
        // hardware not responding
        return false;
    }

    return true;
}

template<class EthImpl>
bool ethInitStatic(EthImpl& eth, IPAddress IP, IPAddress gateway, IPAddress netmask, IPAddress dns1,
                   IPAddress dns2 = IPADDR_NONE)
{
    SPI4EthInit();

    if (!eth.config(IP, gateway, netmask, dns1, dns2))
    {
        // invalid arguments
        return false;
    }

    if (!eth.begin())
    {
        // hardware not responding
        return false;
    }

    return true;
}

#endif  // __LWIPETHERNET_H
