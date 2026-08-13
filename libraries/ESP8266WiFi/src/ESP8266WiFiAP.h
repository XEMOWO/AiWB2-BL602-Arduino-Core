/*
  ESP8266WiFiAP.h - esp8266 Wifi soft-AP class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): API 1:1 with the ESP8266 core. Backend is the
  SDK's wifi_mgmr AP functions (wifi_mgmr_ap_start/ip_set/sta_cnt_get/...).
 */
#ifndef ESP8266WIFIAP_H_
#define ESP8266WIFIAP_H_

#include "ESP8266WiFiType.h"
#include "ESP8266WiFiGeneric.h"

#include "LwipDhcpServer.h"

class ESP8266WiFiAPClass {
public:
    bool softAP(const char* ssid, const char* psk = NULL, int channel = 1, int ssid_hidden = 0, int max_connection = 4, int beacon_interval = 100);
    bool softAP(const String& ssid, const String& psk = emptyString, int channel = 1, int ssid_hidden = 0, int max_connection = 4, int beacon_interval = 100);
    bool softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet);
    bool softAPdisconnect(bool wifioff = false);

    uint8_t softAPgetStationNum();

    IPAddress softAPIP();
    IPAddress softAPbroadcastIP();

    uint8_t* softAPmacAddress(uint8_t* mac);
    String softAPmacAddress(void);

    String softAPSSID() const;
    String softAPPSK() const;

    static DhcpServer& softAPDhcpServer();
};

#endif /* ESP8266WIFIAP_H_ */
