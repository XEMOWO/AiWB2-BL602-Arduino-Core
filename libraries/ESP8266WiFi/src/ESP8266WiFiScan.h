/*
  ESP8266WiFiScan.h - esp8266 Wifi scan class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): API 1:1 with the ESP8266 core. The scan
  backend is the SDK's blocking wifi_mgmr_all_ap_scan(); the async variants
  reuse the same call inside a task (see ESP8266WiFiScan.cpp).

  `bss_info` is the scan-record struct. On ESP8266 it is the nonos-SDK type;
  here it maps 1:1 onto the SDK's wifi_mgmr_ap_item_t fields so
  getScanInfoByIndex() callers keep working.
 */
#ifndef ESP8266WIFISCAN_H_
#define ESP8266WIFISCAN_H_

#include "ESP8266WiFiType.h"
#include "ESP8266WiFiGeneric.h"

#include <functional>

/* Scan record (see header comment for the provenance). The phy_11b/11g/11n
 * and wps fields mirror the ESP8266 nonos-SDK bss_info (WiFiScan.ino reads
 * them); the BL602 scan reports no per-AP PHY mode, so they default to 11g. */
typedef struct bss_info {
    char ssid[33];
    uint8_t bssid[6];
    uint8_t channel;
    int8_t rssi;
    uint8_t authmode;   /* wifi_auth_mode_t-like value from the scan */
    bool is_hidden;     /* true == hidden */
    uint8_t phy_11b;    /* 0/1 — supports 802.11b */
    uint8_t phy_11g;    /* 0/1 — supports 802.11g */
    uint8_t phy_11n;    /* 0/1 — supports 802.11n */
    uint8_t wps;        /* 0/1 — WPS capable */
} bss_info;

class ESP8266WiFiScanClass {
public:
    int8_t scanNetworks(bool async = false, bool show_hidden = false, uint8_t channel = 0, uint8_t* ssid = NULL);
    void scanNetworksAsync(std::function<void(int)> onComplete, bool show_hidden = false);

    int8_t scanComplete();
    void scanDelete();

    // scan result
    const bss_info* getScanInfoByIndex(int i);
    bool getNetworkInfo(uint8_t networkItem, String &ssid, uint8_t &encryptionType, int32_t &RSSI, uint8_t* &BSSID, int32_t &channel, bool &isHidden);

    String SSID(uint8_t networkItem);
    uint8_t encryptionType(uint8_t networkItem);
    int32_t RSSI(uint8_t networkItem);
    uint8_t * BSSID(uint8_t networkItem);
    uint8_t * BSSID(uint8_t networkItem, uint8_t* bssid);
    String BSSIDstr(uint8_t networkItem);
    int32_t channel(uint8_t networkItem);
    bool isHidden(uint8_t networkItem);

protected:
    static bool _scanAsync;
    static bool _scanStarted;
    static bool _scanComplete;

    static size_t _scanCount;
    static void* _scanResult;          /* bss_info[] copy, owned by this class */

    static std::function<void(int)> _onComplete;

    static void _scanDone(void* result, int status);
    static void* _getScanInfoByIndex(int i);

    /* Runs the blocking SDK scan inside a bl_os_task (async scanNetworks). */
    static void _scanTask(void* arg);
};

#endif /* ESP8266WIFISCAN_H_ */
