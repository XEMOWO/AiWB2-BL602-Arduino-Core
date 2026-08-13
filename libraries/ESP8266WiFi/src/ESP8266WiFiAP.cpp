/*
  ESP8266WiFiAP.cpp - esp8266 Wifi soft-AP class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): backend is the SDK's wifi_mgmr AP path.
  wifi_mgmr_ap_start() already brings up the built-in DHCP server (use_dhcp=1);
  the lease pool is sized with wifi_mgmr_ap_dhcp_range_set() like the ESP8266
  default (router .1, pool .100-.200). The DHCP server only exists while the
  AP is up, so DhcpServer::begin()/end() toggle the SDK DHCP flags.
 */
#include "WB2WiFiCommon.h"
#include "ESP8266WiFi.h"     /* `WiFi` instance + aggregate class */
#include "ESP8266WiFiAP.h"

static String s_ap_ssid;
static String s_ap_psk;

/* Single global DHCP-server object (see LwipDhcpServer.h). */
DhcpServer dhcpSoftAP;

/* ---- DhcpServer (control-plane shim over the SDK AP DHCP) ---- */

bool DhcpServer::begin(IPAddress ip, IPAddress netmask)
{
    _started = true;
    _netmask = netmask;
    memcpy(_poolStart, ip.raw_address(), 4);
    _poolStart[3] += 100;              /* pool .x.100 .. .x.200 */
    memcpy(_poolEnd, ip.raw_address(), 4);
    _poolEnd[3] += 200;
    memcpy(_lease, _poolStart, 4);
    memcpy(_leaseEnd, _poolEnd, 4);

    if (wb2_wifi_started && wb2_ap_iface) {
        wifi_mgmr_ap_dhcp_enable();
    }
    return true;
}

void DhcpServer::end()
{
    _started = false;
    if (wb2_wifi_started && wb2_ap_iface) {
        wifi_mgmr_ap_dhcp_disable();
    }
}

void DhcpServer::tick()
{
    /* the SDK drives its own DHCP server; nothing to tick */
}

void DhcpServer::reset()
{
    _started = true;
}

bool DhcpServer::add_dhcps_lease(uint8_t* macaddr)
{
    /* The SDK's wifi_mgmr AP DHCP has no per-MAC static-lease table; accepting
     * the lease keeps StaticLease-style sketches compiling and running. */
    (void)macaddr;
    return true;
}

bool DhcpServer::remove_dhcps_lease(uint8_t* macaddr)
{
    (void)macaddr;
    return true;
}

netif* DhcpServer::getNetif() const
{
    /* CustomOffer reads netif_ip4_addr(getNetif()) to build its captive-portal
     * URI, so hand out a stable netif whose ip_addr tracks the current softAP
     * IP. The SDK's AP DHCP lives inside wifi_mgmr (no lwIP dhcps pbuf state),
     * so this is a lightweight stand-in rather than the real AP netif. */
    static netif s_netif;
    if (wb2_wifi_started) {
        uint32_t ip = 0, gw = 0, mask = 0;
        if (wifi_mgmr_ap_ip_get(&ip, &gw, &mask) == 0) {
            s_netif.ip_addr.addr = ip;   /* ip_addr_t == ip4_addr_t == { u32_t addr; } */
        }
    }
    return &s_netif;
}

/* ---- AP functions ---- */

bool ESP8266WiFiAPClass::softAP(const char* ssid, const char* psk, int channel, int ssid_hidden, int max_connection, int beacon_interval)
{
    if (!WiFi.enableAP(true)) {
        return false;
    }

    size_t ssid_len = ssid ? strlen(ssid) : 0;
    if (ssid_len == 0 || ssid_len > 32) {
        return false;
    }

    size_t psk_len = psk ? strlen(psk) : 0;
    if (psk_len > 0 && (psk_len > 64 || psk_len < 8)) {
        return false;
    }

    s_ap_ssid = ssid;
    s_ap_psk = psk ? psk : "";

    (void)beacon_interval;
    if (max_connection > 0) {
        wifi_mgmr_conf_max_sta(max_connection);   /* best-effort */
    }

    /* ESP8266 default softAP IP (192.168.4.1) when none was configured. */
    uint32_t ip = 0, gw = 0, mask = 0;
    wifi_mgmr_ap_ip_get(&ip, &gw, &mask);
    if (ip == 0) {
        wifi_mgmr_ap_ip_set(IPAddress(192, 168, 4, 1).asUint32(),
                            IPAddress(192, 168, 4, 1).asUint32(),
                            IPAddress(255, 255, 255, 0).asUint32());
    }

    int ret = wifi_mgmr_ap_start(&wb2_ap_iface,
                                 (char *)ssid, ssid_hidden,
                                 (char *)(psk ? psk : ""), channel);
    if (ret != 0) {
        return false;
    }

    /* ESP8266-style lease pool (.1 router, .100-.200 leases). */
    wifi_mgmr_ap_dhcp_range_set(ip ? ip : IPAddress(192, 168, 4, 1).asUint32(),
                                IPAddress(255, 255, 255, 0).asUint32(), 100, 200);
    return true;
}

bool ESP8266WiFiAPClass::softAP(const String& ssid, const String& psk, int channel, int ssid_hidden, int max_connection, int beacon_interval)
{
    return softAP(ssid.c_str(), psk.c_str(), channel, ssid_hidden, max_connection, beacon_interval);
}

bool ESP8266WiFiAPClass::softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet)
{
    if (!WiFi.enableAP(true)) {
        return false;
    }

    wifi_mgmr_ap_ip_set(local_ip.asUint32(), gateway.asUint32(), subnet.asUint32());

    /* Size the SDK DHCP pool like ESP8266 (router .1, pool .100-.200). */
    wifi_mgmr_ap_dhcp_range_set(local_ip.asUint32(), subnet.asUint32(), 100, 200);
    wifi_mgmr_ap_dhcp_enable();

    /* Verify the address stuck (some SDK builds normalize the AP netif). */
    uint32_t ip = 0, gw = 0, mask = 0;
    wifi_mgmr_ap_ip_get(&ip, &gw, &mask);
    return ip != 0;
}

bool ESP8266WiFiAPClass::softAPdisconnect(bool wifioff)
{
    bool ret = false;
    if (wb2_ap_iface) {
        ret = (wifi_mgmr_ap_stop(&wb2_ap_iface) == 0);
        wb2_ap_iface = NULL;
    } else {
        ret = true;
    }

    if (ret && wifioff) {
        ret = WiFi.enableAP(false);
    }
    return ret;
}

uint8_t ESP8266WiFiAPClass::softAPgetStationNum()
{
    uint8_t cnt = 0;
    if (wb2_ap_iface) {
        wifi_mgmr_ap_sta_cnt_get(&cnt);
    }
    return cnt;
}

IPAddress ESP8266WiFiAPClass::softAPIP()
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (wb2_ap_iface) {
        wifi_mgmr_ap_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(ip);
}

IPAddress ESP8266WiFiAPClass::softAPbroadcastIP()
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (wb2_ap_iface) {
        wifi_mgmr_ap_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(ip | ~mask);
}

uint8_t* ESP8266WiFiAPClass::softAPmacAddress(uint8_t* mac)
{
    uint8_t m[6] = {0};
    if (wb2_wifi_started) {
        wifi_mgmr_ap_mac_get(m);
    }
    memcpy(mac, m, 6);
    return mac;
}

String ESP8266WiFiAPClass::softAPmacAddress(void)
{
    uint8_t mac[6] = {0};
    char macStr[18] = { 0 };
    if (wb2_wifi_started) {
        wifi_mgmr_ap_mac_get(mac);
    }
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String ESP8266WiFiAPClass::softAPSSID() const
{
    return s_ap_ssid;
}

String ESP8266WiFiAPClass::softAPPSK() const
{
    return s_ap_psk;
}

DhcpServer& ESP8266WiFiAPClass::softAPDhcpServer()
{
    return dhcpSoftAP;
}

/* ---- non-OS-SDK soft-AP config (declared in cores/arduino/user_interface.h)
 *
 * TestEspApi drives the AP through softap_config: it reads the current config,
 * flips authmode OPEN <-> WPA2_PSK, and pushes it back with
 * wifi_softap_set_config(). Backed by the class API above, so both spellings
 * converge on the same wifi_mgmr state.
 */
extern "C" uint8 wifi_softap_get_station_num(void)
{
    return WiFi.softAPgetStationNum();
}

extern "C" bool wifi_softap_get_config(struct softap_config *config)
{
    if (!config) {
        return false;
    }
    memset(config, 0, sizeof(*config));
    size_t n = s_ap_ssid.length();
    if (n > 32) {
        n = 32;
    }
    memcpy(config->ssid, s_ap_ssid.c_str(), n);
    config->ssid_len = (uint8)n;
    n = s_ap_psk.length();
    if (n > 64) {
        n = 64;
    }
    memcpy(config->password, s_ap_psk.c_str(), n);
    config->channel = WiFi.channel();
    config->authmode = s_ap_psk.length() ? AUTH_WPA2_PSK : AUTH_OPEN;
    config->ssid_hidden = 0;
    config->max_connection = 4;
    config->beacon_interval = 100;
    return true;
}

extern "C" bool wifi_softap_set_config(struct softap_config *config)
{
    if (!config) {
        return false;
    }
    return WiFi.softAP((const char*)config->ssid, (const char*)config->password,
                       config->channel, config->ssid_hidden,
                       config->max_connection, config->beacon_interval);
}
