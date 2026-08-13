/*
 * Ethernet.cpp - the classic EthernetClass shim (see Ethernet.h).
 * Everything is static and rides on the WiFi object.
 */

#include "Ethernet.h"

static bool s_eth_ready = false;

int EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet,
                         unsigned long timeout, unsigned long responseTimeout)
{
    (void)mac; (void)responseTimeout;
    WiFi.mode(WIFI_STA);
    if (ip != IPAddress(0, 0, 0, 0)) {
        WiFi.config(ip, gateway, subnet, dns);
    }
    WiFi.begin();

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeout) {
        delay(50);
    }
    s_eth_ready = (WiFi.localIP() != IPAddress(0, 0, 0, 0));
    return s_eth_ready ? 1 : 0;
}

int EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway,
                         unsigned long timeout, unsigned long responseTimeout)
{
    return begin(mac, ip, dns, gateway, IPAddress(255, 255, 255, 0), timeout, responseTimeout);
}

int EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns,
                         unsigned long timeout, unsigned long responseTimeout)
{
    return begin(mac, ip, dns, IPAddress(0, 0, 0, 0), IPAddress(255, 255, 255, 0), timeout, responseTimeout);
}

int EthernetClass::begin(uint8_t *mac, IPAddress ip,
                         unsigned long timeout, unsigned long responseTimeout)
{
    return begin(mac, ip, IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(255, 255, 255, 0),
                 timeout, responseTimeout);
}

int EthernetClass::begin(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout)
{
    return begin(mac, IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0),
                 IPAddress(0, 0, 0, 0), timeout, responseTimeout);
}

IPAddress EthernetClass::localIP() { return WiFi.localIP(); }
IPAddress EthernetClass::subnetMask() { return WiFi.subnetMask(); }
IPAddress EthernetClass::gatewayIP() { return WiFi.gatewayIP(); }
IPAddress EthernetClass::dnsServerIP() { return WiFi.dnsIP(); }

int EthernetClass::maintain() { return 0; }

EthernetLinkStatus EthernetClass::linkStatus() { return LinkON; }
EthernetHardwareStatus EthernetClass::hardwareStatus() { return EthernetW5500; }

EthernetClass Ethernet;
