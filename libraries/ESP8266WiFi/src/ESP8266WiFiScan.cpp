/*
  ESP8266WiFiScan.cpp - esp8266 Wifi scan class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): the SDK's wifi_mgmr_all_ap_scan() is blocking,
  so the synchronous scanNetworks() calls it directly and the async variants
  run the same call inside a bl_os_task. Results are converted into the
  ESP8266 bss_info record (authmode mapped from the SDK auth value).
 */
#include "WB2WiFiCommon.h"
#include "ESP8266WiFi.h"     /* `WiFi` instance + aggregate class */
#include "ESP8266WiFiScan.h"

bool ESP8266WiFiScanClass::_scanAsync = false;
bool ESP8266WiFiScanClass::_scanStarted = false;
bool ESP8266WiFiScanClass::_scanComplete = false;

size_t ESP8266WiFiScanClass::_scanCount = 0;
void* ESP8266WiFiScanClass::_scanResult = NULL;

std::function<void(int)> ESP8266WiFiScanClass::_onComplete;

/* Convert an SDK scan result array (wifi_mgmr_ap_item_t[]) into the private
 * bss_info[] copy. `result`/`status` follow the ESP8266 _scanDone() shape:
 * result == item array, status == item count. */
void ESP8266WiFiScanClass::_scanDone(void* result, int status)
{
    if (_scanResult) {
        bl_os_free(_scanResult);
        _scanResult = NULL;
    }
    _scanCount = 0;

    if (status <= 0 || !result) {
        return;
    }

    wifi_mgmr_ap_item_t *ary = (wifi_mgmr_ap_item_t *)result;
    uint32_t num = (uint32_t)status;

    _scanResult = bl_os_malloc(sizeof(bss_info) * num);
    if (!_scanResult) {
        return;
    }

    bss_info *dst = (bss_info *)_scanResult;
    for (uint32_t i = 0; i < num; i++) {
        memset(&dst[i], 0, sizeof(bss_info));
        memcpy(dst[i].ssid, ary[i].ssid, 32);
        dst[i].ssid[32] = 0;
        memcpy(dst[i].bssid, ary[i].bssid, 6);
        dst[i].channel = ary[i].channel;
        dst[i].rssi = ary[i].rssi;
        dst[i].authmode = ary[i].auth;
        dst[i].is_hidden = false;   /* wifi_mgmr doesn't report the hidden flag */
        /* BL602 reports no per-AP PHY mode; default to 802.11g (universal). */
        dst[i].phy_11g = 1;
    }
    _scanCount = num;
}

/* Run the blocking scan + conversion, then hand control back to the async
 * caller (task context). */
void ESP8266WiFiScanClass::_scanTask(void *arg)
{
    (void)arg;
    wifi_mgmr_ap_item_t *ary = NULL;
    uint32_t num = 0;

    if (wifi_mgmr_all_ap_scan(&ary, &num) != 0) {
        num = 0;
    }

    ESP8266WiFiScanClass::_scanDone(ary, (int)num);
    if (ary) {
        bl_os_free(ary);
    }

    ESP8266WiFiScanClass::_scanStarted = false;
    ESP8266WiFiScanClass::_scanComplete = true;

    if (ESP8266WiFiScanClass::_onComplete) {
        ESP8266WiFiScanClass::_onComplete((int)ESP8266WiFiScanClass::_scanCount);
    }

    bl_os_task_delete(NULL);
}

int8_t ESP8266WiFiScanClass::scanNetworks(bool async, bool show_hidden, uint8_t channel, uint8_t* ssid)
{
    (void)show_hidden;
    (void)channel;
    (void)ssid;

    if (_scanStarted) {
        return WIFI_SCAN_RUNNING;
    }

    _scanAsync = async;

    WiFi.enableSTA(true);

    scanDelete();

    if (async) {
        _scanStarted = true;
        _scanComplete = false;
        if (bl_os_task_create("wb2scan", (void *)&ESP8266WiFiScanClass::_scanTask, 1024 * 4, NULL, 10, NULL) != 0) {
            _scanStarted = false;
            return WIFI_SCAN_FAILED;
        }
        return WIFI_SCAN_RUNNING;
    }

    /* synchronous: blocking call in the caller's context */
    wifi_mgmr_ap_item_t *ary = NULL;
    uint32_t num = 0;
    if (wifi_mgmr_all_ap_scan(&ary, &num) != 0) {
        num = 0;
    }
    _scanDone(ary, (int)num);
    if (ary) {
        bl_os_free(ary);
    }
    _scanComplete = true;
    return (int8_t)_scanCount;
}

void ESP8266WiFiScanClass::scanNetworksAsync(std::function<void(int)> onComplete, bool show_hidden)
{
    _onComplete = onComplete;
    scanNetworks(true, show_hidden);
}

int8_t ESP8266WiFiScanClass::scanComplete()
{
    if (_scanStarted) {
        return WIFI_SCAN_RUNNING;
    }
    if (_scanComplete) {
        return (int8_t)_scanCount;
    }
    return WIFI_SCAN_FAILED;
}

void ESP8266WiFiScanClass::scanDelete()
{
    if (_scanResult) {
        bl_os_free(_scanResult);
        _scanResult = NULL;
        _scanCount = 0;
    }
    _scanComplete = false;
}

void* ESP8266WiFiScanClass::_getScanInfoByIndex(int i)
{
    if (i < 0 || (size_t)i >= _scanCount || !_scanResult) {
        return NULL;
    }
    return &((bss_info *)_scanResult)[i];
}

const bss_info *ESP8266WiFiScanClass::getScanInfoByIndex(int i)
{
    return (const bss_info *)_getScanInfoByIndex(i);
}

bool ESP8266WiFiScanClass::getNetworkInfo(uint8_t networkItem, String &ssid, uint8_t &encryptionType, int32_t &RSSI, uint8_t* &BSSID, int32_t &channel, bool &isHidden)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return false;
    }

    char ssid_copy[33];
    memcpy(ssid_copy, it->ssid, sizeof(it->ssid));
    ssid_copy[32] = 0;
    ssid = (const char*) ssid_copy;
    encryptionType = this->encryptionType(networkItem);
    RSSI = it->rssi;
    BSSID = it->bssid;
    channel = it->channel;
    isHidden = (it->is_hidden != 0);

    return true;
}

String ESP8266WiFiScanClass::SSID(uint8_t networkItem)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return String();
    }
    char tmp[33];
    memcpy(tmp, it->ssid, sizeof(it->ssid));
    tmp[32] = 0;
    return String((const char*) tmp);
}

uint8_t ESP8266WiFiScanClass::encryptionType(uint8_t networkItem)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return -1;
    }

    switch (it->authmode) {
    case 1:  return ENC_TYPE_NONE;   /* wifi_mgmr_ap_auth_mode_t OPEN */
    case 2:  return ENC_TYPE_WEP;
    case 3:  return ENC_TYPE_TKIP;   /* WPA_PSK */
    case 4:  return ENC_TYPE_CCMP;   /* WPA2_PSK */
    case 5:  return ENC_TYPE_AUTO;   /* WPA_WPA2_PSK */
    default: return -1;
    }
}

int32_t ESP8266WiFiScanClass::RSSI(uint8_t networkItem)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return 0;
    }
    return it->rssi;
}

uint8_t* ESP8266WiFiScanClass::BSSID(uint8_t networkItem)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return NULL;
    }
    return it->bssid;
}

uint8_t* ESP8266WiFiScanClass::BSSID(uint8_t networkItem, uint8_t* bssid)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return NULL;
    }
    memcpy(bssid, it->bssid, WL_MAC_ADDR_LENGTH);
    return bssid;
}

String ESP8266WiFiScanClass::BSSIDstr(uint8_t networkItem)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return String();
    }
    char mac[18] = { 0 };
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             it->bssid[0], it->bssid[1], it->bssid[2],
             it->bssid[3], it->bssid[4], it->bssid[5]);
    return String(mac);
}

int32_t ESP8266WiFiScanClass::channel(uint8_t networkItem)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return 0;
    }
    return it->channel;
}

bool ESP8266WiFiScanClass::isHidden(uint8_t networkItem)
{
    bss_info *it = (bss_info *)_getScanInfoByIndex(networkItem);
    if (!it) {
        return false;
    }
    return (it->is_hidden != 0);
}
