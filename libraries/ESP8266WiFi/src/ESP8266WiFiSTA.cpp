/*
  ESP8266WiFiSTA.cpp - esp8266 Wifi station class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): backend is the SDK's wifi_mgmr. The ESP8266
  station_config semantics map onto wifi_mgmr_sta_connect(); static IP maps
  onto wifi_mgmr_sta_ip_set()/dns_get(). IP values are network-order dwords,
  which the core's IPAddress stores as-is, so asUint32() round-trips.
 */
#include "WB2WiFiCommon.h"
#include "ESP8266WiFi.h"     /* `WiFi` instance + aggregate class */
#include "ESP8266WiFiSTA.h"

bool ESP8266WiFiSTAClass::_useStaticIp = false;
bool ESP8266WiFiSTAClass::_useInsecureWEP = false;
bool ESP8266WiFiSTAClass::_smartConfigStarted = false;
bool ESP8266WiFiSTAClass::_smartConfigDone = false;

static void s_wb2_cache_ssid(const char *ssid)
{
    /* wifi_mgmr keeps the ssid in its connect_ind_stat; nothing else needed. */
    (void)ssid;
}

wl_status_t ESP8266WiFiSTAClass::begin(const char* ssid, const char *passphrase, int32_t channel, const uint8_t* bssid, bool connect)
{
    if (!WiFi.enableSTA(true)) {
        return WL_CONNECT_FAILED;
    }

    if (!ssid || *ssid == 0x00 || strlen(ssid) > 32) {
        return WL_CONNECT_FAILED;
    }

    int passphraseLen = passphrase ? strlen(passphrase) : 0;
    if (passphraseLen > 64) {
        return WL_WRONG_PASSWORD;
    }

    (void)connect;   /* connect=true is our only mode: begin() always connects */

    /* The SDK stores the last AP itself; ip_set with dhcp comes via connect_mid
       (use_dhcp=1). A BSSID/channel restriction is not exposed by wifi_mgmr's
       connect path, so those args are accepted but not honored (documented
       deviation). */
    s_wb2_cache_ssid(ssid);

    wifi_mgmr_sta_connect(&wb2_sta_iface,
                          (char *)ssid,
                          (char *)(passphrase ? passphrase : ""),
                          NULL, (uint8_t *)bssid, 0, (uint8_t)channel);

    if (!_useStaticIp) {
        /* connect_mid above used use_dhcp=1, so DHCP is already on. */
    }

    return status();
}

wl_status_t ESP8266WiFiSTAClass::begin(char* ssid, char *passphrase, int32_t channel, const uint8_t* bssid, bool connect)
{
    return begin((const char*) ssid, (const char*) passphrase, channel, bssid, connect);
}

wl_status_t ESP8266WiFiSTAClass::begin(const String& ssid, const String& passphrase, int32_t channel, const uint8_t* bssid, bool connect)
{
    return begin(ssid.c_str(), passphrase.c_str(), channel, bssid, connect);
}

wl_status_t ESP8266WiFiSTAClass::begin()
{
    if (!WiFi.enableSTA(true)) {
        return WL_CONNECT_FAILED;
    }

    /* Re-run the stored-AP connect that wifi_mgmr keeps in its profile. */
    wifi_mgmr_sta_autoconnect_enable();
    return status();
}

bool ESP8266WiFiSTAClass::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2)
{
    if (!WiFi.enableSTA(true)) {
        return false;
    }

    if (local_ip == INADDR_ANY && gateway == INADDR_ANY && subnet == INADDR_ANY) {
        _useStaticIp = false;
        return true;   /* fall back to DHCP */
    }

    uint32_t ip = local_ip.asUint32();
    uint32_t mask = subnet.asUint32();
    uint32_t gw = gateway.asUint32();
    uint32_t d1 = dns1.asUint32();
    uint32_t d2 = dns2.asUint32();

    if (wifi_mgmr_sta_ip_set(ip, mask, gw, d1, d2) == 0) {
        _useStaticIp = true;
        return true;
    }
    return false;
}

bool ESP8266WiFiSTAClass::config(IPAddress local_ip, IPAddress dns)
{
    if (!local_ip.asUint32()) {
        return config(INADDR_ANY, INADDR_ANY, INADDR_ANY);
    }

    IPAddress gw(local_ip);
    gw[3] = 1;
    if (!dns.asUint32()) {
        dns = gw;
    }
    return config(local_ip, dns, gw);
}

bool ESP8266WiFiSTAClass::setDNS(IPAddress dns1, IPAddress dns2)
{
    if ((WiFi.getMode() & WIFI_STA) == 0) {
        return false;
    }

    if (dns1.asUint32()) {
        ip_addr_t a;
        ip_addr_set_ip4_u32(&a, dns1.asUint32());
        dns_setserver(0, &a);
    }
    if (dns2.asUint32()) {
        ip_addr_t a;
        ip_addr_set_ip4_u32(&a, dns2.asUint32());
        dns_setserver(1, &a);
    }
    return true;
}

bool ESP8266WiFiSTAClass::reconnect()
{
    if ((WiFi.getMode() & WIFI_STA) != 0) {
        wifi_mgmr_sta_disconnect();
        return wifi_mgmr_sta_autoconnect_enable() == 0;
    }
    return false;
}

bool ESP8266WiFiSTAClass::disconnect(bool wifioff)
{
    return disconnect(wifioff, true);
}

bool ESP8266WiFiSTAClass::disconnect(bool wifioff, bool eraseCredentials)
{
    bool ret = false;

    /* wifi_mgmr forgets the credentials only when told to re-provision; the
       ESP8266 "erase saved creds" flag has no direct equivalent, so it is
       honored as a disconnect. */
    (void)eraseCredentials;

    if (WiFi.getMode() & WIFI_STA) {
        ret = (wifi_mgmr_sta_disconnect() == 0);
    } else {
        ret = true;
    }

    if (wifioff) {
        WiFi.enableSTA(false);
    }

    return ret;
}

bool ESP8266WiFiSTAClass::isConnected()
{
    return (status() == WL_CONNECTED);
}

bool ESP8266WiFiSTAClass::setAutoConnect(bool autoConnect)
{
    if (autoConnect) {
        return wifi_mgmr_sta_autoconnect_enable() == 0;
    }
    return wifi_mgmr_sta_autoconnect_disable() == 0;
}

bool ESP8266WiFiSTAClass::getAutoConnect()
{
    /* wifi_mgmr does not expose the stored autoconnect flag; it is on by
       default once the radio is up. */
    return true;
}

bool ESP8266WiFiSTAClass::setAutoReconnect(bool autoReconnect)
{
    if (autoReconnect) {
        return wifi_mgmr_sta_autoconnect_set(3, -1) == 0;   /* retry every 3s */
    }
    return wifi_mgmr_sta_autoconnect_disable() == 0;
}

bool ESP8266WiFiSTAClass::getAutoReconnect()
{
    return true;
}

int8_t ESP8266WiFiSTAClass::waitForConnectResult(unsigned long timeoutLength)
{
    if ((WiFi.getMode() & WIFI_STA) == 0) {
        return WL_DISCONNECTED;
    }

    unsigned long start = millis();
    while ((unsigned long)(millis() - start) < timeoutLength) {
        wl_status_t st = status();
        if (st != WL_DISCONNECTED && st != WL_IDLE_STATUS) {
            return st;
        }
        delay(10);
    }
    return -1;   /* -1 indicates timeout */
}

IPAddress ESP8266WiFiSTAClass::localIP()
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (wb2_wifi_started) {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(ip);
}

uint8_t* ESP8266WiFiSTAClass::macAddress(uint8_t* mac)
{
    uint8_t m[6] = {0};
    if (wb2_wifi_started) {
        wifi_mgmr_sta_mac_get(m);
    }
    memcpy(mac, m, 6);
    return mac;
}

String ESP8266WiFiSTAClass::macAddress(void)
{
    uint8_t mac[6] = {0};
    char macStr[18] = { 0 };
    if (wb2_wifi_started) {
        wifi_mgmr_sta_mac_get(mac);
    }
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

IPAddress ESP8266WiFiSTAClass::subnetMask()
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (wb2_wifi_started) {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(mask);
}

IPAddress ESP8266WiFiSTAClass::gatewayIP()
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (wb2_wifi_started) {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(gw);
}

IPAddress ESP8266WiFiSTAClass::dnsIP(uint8_t dns_no)
{
    uint32_t dns1 = 0, dns2 = 0;
    if (wb2_wifi_started) {
        wifi_mgmr_sta_dns_get(&dns1, &dns2);
    }
    return IPAddress(dns_no == 0 ? dns1 : dns2);
}

IPAddress ESP8266WiFiSTAClass::broadcastIP()
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (wb2_wifi_started) {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(ip | ~mask);
}

wl_status_t ESP8266WiFiSTAClass::status()
{
    return wb2_sta_wl_status();
}

String ESP8266WiFiSTAClass::SSID() const
{
    wifi_mgmr_sta_connect_ind_stat_info_t st;
    memset(&st, 0, sizeof(st));
    wifi_mgmr_sta_connect_ind_stat_get(&st);
    if (st.ssid[0]) {
        return String(st.ssid);
    }
    return String();
}

String ESP8266WiFiSTAClass::psk() const
{
    wifi_mgmr_sta_connect_ind_stat_info_t st;
    memset(&st, 0, sizeof(st));
    wifi_mgmr_sta_connect_ind_stat_get(&st);
    if (st.passphr[0]) {
        return String(st.passphr);
    }
    return String();
}

uint8_t* ESP8266WiFiSTAClass::BSSID(void)
{
    static uint8_t s_bssid[6] = {0};
    wifi_mgmr_sta_connect_ind_stat_info_t st;
    memset(&st, 0, sizeof(st));
    wifi_mgmr_sta_connect_ind_stat_get(&st);
    memcpy(s_bssid, st.bssid, 6);
    return s_bssid;
}

uint8_t* ESP8266WiFiSTAClass::BSSID(uint8_t* bssid)
{
    wifi_mgmr_sta_connect_ind_stat_info_t st;
    memset(&st, 0, sizeof(st));
    wifi_mgmr_sta_connect_ind_stat_get(&st);
    memcpy(bssid, st.bssid, WL_MAC_ADDR_LENGTH);
    return bssid;
}

String ESP8266WiFiSTAClass::BSSIDstr(void)
{
    wifi_mgmr_sta_connect_ind_stat_info_t st;
    memset(&st, 0, sizeof(st));
    wifi_mgmr_sta_connect_ind_stat_get(&st);
    char mac[18] = { 0 };
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             st.bssid[0], st.bssid[1], st.bssid[2],
             st.bssid[3], st.bssid[4], st.bssid[5]);
    return String(mac);
}

int8_t ESP8266WiFiSTAClass::RSSI(void)
{
    int rssi = 0;
    if (wb2_wifi_started) {
        wifi_mgmr_rssi_get(&rssi);
    }
    return (int8_t)rssi;
}

/* ---- STA remote configure (BL602 has neither WPS nor SmartConfig) ---- */

bool ESP8266WiFiSTAClass::beginWPSConfig(void)
{
    return false;
}

bool ESP8266WiFiSTAClass::beginSmartConfig()
{
    if (_smartConfigStarted) {
        return false;
    }
    _smartConfigStarted = true;
    return false;
}

bool ESP8266WiFiSTAClass::stopSmartConfig()
{
    bool wasStarted = _smartConfigStarted;
    _smartConfigStarted = false;
    return wasStarted;
}

bool ESP8266WiFiSTAClass::smartConfigDone()
{
    return _smartConfigDone;
}
