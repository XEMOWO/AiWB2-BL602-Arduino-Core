/*
  LwipIntfDev.h — compile-compatible lwIP Ethernet device for the Ai-WB2-12F (BL602).

  The ESP8266 core's LwipIntfDev<RawDev> template wires a raw Ethernet MAC
  (Wiznet W5100/W5500, ENC28J60) into the lwIP stack: it allocates a netif,
  drives a DHCP client and polls the MAC for frames. The WB2 has no SPI
  Ethernet hardware and its lwIP build has no second wired interface, so this
  header provides the same template with a no-op body: `begin()` always reports
  "hardware not responding" and no netif is ever created. Every ESP8266
  sketch/library that drives a LwipIntfDev (the lwIP_Ethernet / lwIP_w5x00 /
  lwIP_enc28j60 examples) therefore compiles unchanged and at runtime prints
  "no hardware found" then idles — the "hardware-impossible = harmless no-op"
  rule.

  The public API mirrors the reference (cores/esp8266/LwipIntfDev.h) 1:1 so
  header-level compatibility holds; the raw-device hooks (isLinked /
  isLinkDetectable / begin) are provided by the driver classes in the
  lwIP_w5x00 / lwIP_enc28j60 libraries.
*/

#ifndef _LWIPINTFDEV_H
#define _LWIPINTFDEV_H

#include <stdint.h>
#include <string.h>
#include <Arduino.h>
#include <IPAddress.h>
#include <wl_definitions.h>
#include <SPI.h>

/* IPADDR_NONE is a lwIP macro (ip4_addr.h). ESP8266WiFi.h already pulls in the
 * lwIP headers for most sketches; pull them in directly so that including
 * <W5500lwIP.h> alone (as the reference allows) also works. Skip if a prior
 * include already defined it, to avoid re-introducing INADDR_* macros that
 * lwip_compat_undef.h has already neutralized. */
#ifndef IPADDR_NONE
#include <lwip/ip4_addr.h>
#endif

#ifndef DEFAULT_MTU
#define DEFAULT_MTU 1500
#endif

/* Arduino Ethernet compatibility enum (see LwipIntfDev::linkStatus()).
 * Guarded against the classic libraries/Ethernet port, whose Ethernet.h
 * declares the same enum with the same enumerators. */
#if !defined(__ETHERNET_H__)
enum EthernetLinkStatus
{
    Unknown,
    LinkON,
    LinkOFF
};
#endif

template<class RawDev>
class LwipIntfDev : public RawDev
{
public:
    LwipIntfDev(int8_t cs = SS, SPIClass& spi = SPI, int8_t intr = -1) :
        RawDev(cs, spi, intr), _mtu(DEFAULT_MTU), _started(false)
    {
        memset(_macAddress, 0, sizeof _macAddress);
    }

    // ESP argument order: (local_ip, gateway, netmask, dns1[, dns2]).
    // The Arduino legacy order is reordered before we get here by
    // ArduinoEthernet in EthernetCompat.h.
    boolean config(const IPAddress& localIP, const IPAddress& gateway, const IPAddress& netmask,
                   const IPAddress& dns1 = IPADDR_NONE, const IPAddress& dns2 = IPADDR_NONE)
    {
        (void)dns2;
        if (_started)
        {
            return false;  // "use config() then begin()"
        }
        _ip   = localIP;
        _gw   = gateway;
        _mask = netmask;
        _dns1 = dns1;
        return true;
    }

    // legacy two-parameter form, 2nd param is DNS (like Arduino)
    boolean config(IPAddress localIP, IPAddress dns = INADDR_ANY)
    {
        if (!localIP.isSet())
        {
            return config(INADDR_ANY, INADDR_ANY, INADDR_ANY);
        }
        IPAddress gw(localIP);
        gw[3] = 1;
        return config(localIP, gw, IPAddress(255, 255, 255, 0), dns);
    }

    // No SPI Ethernet on WB2: the raw device's begin() reports "not
    // responding", so this always returns false and no netif is created.
    boolean begin(const uint8_t* macAddress = nullptr, const uint16_t mtu = DEFAULT_MTU)
    {
        if (mtu)
        {
            _mtu = mtu;
        }
        if (macAddress)
        {
            memcpy(_macAddress, macAddress, 6);
        }
        _started = false;
        return RawDev::begin(_macAddress);
    }

    void end()
    {
        _started = false;
        RawDev::end();
    }

    uint8_t* macAddress(uint8_t* mac)
    {
        memcpy(mac, _macAddress, 6);
        return mac;
    }

    IPAddress localIP() const
    {
        return _ip;
    }
    IPAddress subnetMask() const
    {
        return _mask;
    }
    IPAddress gatewayIP() const
    {
        return _gw;
    }
    IPAddress dnsIP(int n = 0) const
    {
        (void)n;
        return _dns1;
    }

    void setDNS(IPAddress dns1, IPAddress dns2 = INADDR_ANY)
    {
        (void)dns2;
        if (dns1.isSet())
        {
            _dns1 = dns1;
        }
    }

    void setDefault(bool deflt = true)
    {
        (void)deflt;  // routing is WiFi-only on WB2
    }

    // never started on WB2 (begin() always fails), so always false
    bool connected()
    {
        return _started && _ip.isSet();
    }

    bool routable()
    {
        return false;
    }

    // ESP8266WiFi API compatibility
    wl_status_t status()
    {
        return _started ? (connected() ? WL_CONNECTED : WL_DISCONNECTED) : WL_NO_SHIELD;
    }

    // Arduino Ethernet compatibility
    EthernetLinkStatus linkStatus()
    {
        return RawDev::isLinkDetectable() ? (_started && RawDev::isLinked() ? LinkON : LinkOFF)
                                          : Unknown;
    }

protected:
    IPAddress _ip;       // static or DHCP-assigned local address
    IPAddress _gw;       // gateway
    IPAddress _mask;     // netmask
    IPAddress _dns1;     // primary DNS
    uint8_t   _macAddress[6];
    uint16_t  _mtu;
    bool      _started;
};

#endif  // _LWIPINTFDEV_H
