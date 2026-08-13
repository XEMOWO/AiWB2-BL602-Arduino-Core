/*
 * Ethernet.h - classic Arduino Ethernet library for the Ai-WB2-12F (BL602).
 *
 * Compile-compatible port. The WB2 has no Wiznet/Ethernet hardware; the whole
 * classic Ethernet API (EthernetClass/EthernetClient/EthernetServer/EthernetUDP)
 * is a shim that rides on the WiFi + lwIP stack:
 *   - Ethernet.begin(mac[, ip, dns, gateway, subnet]) starts STA mode and
 *     obtains an IP (WiFi.begin(), or WiFi.config() for the static forms).
 *   - hardwareStatus() reports a Wiznet present (so the examples don't spin on
 *     "no shield"); linkStatus() reports ON.
 *   - Clients/servers/UDP are WiFiClient/WiFiServer/WiFiUDP under the hood.
 *
 * The classic examples therefore compile unchanged; at runtime they talk over
 * WiFi instead of an Ethernet shield.
 */

#ifndef __ETHERNET_H__
#define __ETHERNET_H__

#include <Arduino.h>
#include <IPAddress.h>
#include <ESP8266WiFi.h>

#include "EthernetClient.h"
#include "EthernetServer.h"
#include "EthernetUdp.h"

/* ---- classic status enums (libraries/Ethernet/src/utility/w5100.h) ---- */

typedef enum EthernetHardwareStatus {
    EthernetNoHardware,
    EthernetW5100,
    EthernetW5200,
    EthernetW5500
} EthernetHardwareStatus;

typedef enum EthernetLinkStatus {
    Unknown,
    LinkON,
    LinkOFF
} EthernetLinkStatus;

class EthernetClass
{
public:
    static int begin(uint8_t *mac, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    static int begin(uint8_t *mac, IPAddress ip, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    static int begin(uint8_t *mac, IPAddress ip, IPAddress dns, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    static int begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    static int begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);

    static void init(uint8_t csPin) { (void)csPin; }

    static IPAddress localIP();
    static IPAddress subnetMask();
    static IPAddress gatewayIP();
    static IPAddress dnsServerIP();

    static void setMACAddress(const uint8_t *mac) { (void)mac; }
    static void setLocalIP(const IPAddress local_ip) { (void)local_ip; }
    static void setSubnetMask(const IPAddress subnet) { (void)subnet; }
    static void setGatewayIP(const IPAddress gateway) { (void)gateway; }
    static void setDnsServerIP(const IPAddress dns_server) { (void)dns_server; }

    static int maintain();

    static EthernetLinkStatus linkStatus();
    static EthernetHardwareStatus hardwareStatus();
};

extern EthernetClass Ethernet;

#endif /* __ETHERNET_H__ */
