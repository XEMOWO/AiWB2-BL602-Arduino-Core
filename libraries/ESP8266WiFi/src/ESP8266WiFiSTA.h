/*
  ESP8266WiFiSTA.h - esp8266 Wifi station class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): API 1:1 with the ESP8266 core. The ESP8266
  STA class derives from a core-internal LwipIntf helper (broadcastIP/...);
  those methods are inlined here instead so the header stays self-contained.
  Backend is wifi_mgmr (see ESP8266WiFiSTA.cpp).
 */
#ifndef ESP8266WIFISTA_H_
#define ESP8266WIFISTA_H_

#include "ESP8266WiFiType.h"
#include "ESP8266WiFiGeneric.h"

class ESP8266WiFiSTAClass {
public:
    wl_status_t begin(const char* ssid, const char *passphrase = NULL, int32_t channel = 0, const uint8_t* bssid = NULL, bool connect = true);
    wl_status_t begin(char* ssid, char *passphrase = NULL, int32_t channel = 0, const uint8_t* bssid = NULL, bool connect = true);
    wl_status_t begin(const String& ssid, const String& passphrase = emptyString, int32_t channel = 0, const uint8_t* bssid = NULL, bool connect = true);
    wl_status_t begin();

    // The argument order is ESP's (same as ESP8266); Arduino-compat 2/1-arg
    // overloads are deliberately not provided (see reference core).
    bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = INADDR_ANY, IPAddress dns2 = INADDR_ANY);
    [[deprecated("It is discouraged to use this 1 or 2 parameters network configuration legacy function config(ip[,dns]) as chosen defaults may not match the local network configuration")]]
    bool config(IPAddress local_ip, IPAddress dns = INADDR_ANY);

    bool setDNS(IPAddress dns1, IPAddress dns2 = INADDR_ANY);

    bool reconnect();

    bool disconnect(bool wifioff = false);
    bool disconnect(bool wifioff, bool eraseCredentials);

    bool isConnected();

    bool setAutoConnect(bool autoConnect);
    bool getAutoConnect();

    bool setAutoReconnect(bool autoReconnect);
    bool getAutoReconnect();

    int8_t waitForConnectResult(unsigned long timeoutLength = 60000);

    // STA network info
    IPAddress localIP();

    uint8_t * macAddress(uint8_t* mac);
    String macAddress();

    IPAddress subnetMask();
    IPAddress gatewayIP();
    IPAddress dnsIP(uint8_t dns_no = 0);

    IPAddress broadcastIP();
    // STA WiFi info
    wl_status_t status();
    String SSID() const;
    String psk() const;

    uint8_t * BSSID();
    uint8_t * BSSID(uint8_t* bssid);
    String BSSIDstr();

    int8_t RSSI();

    static void enableInsecureWEP (bool enable = true) { _useInsecureWEP = enable; }

protected:
    static bool _useStaticIp;
    static bool _useInsecureWEP;

    // STA remote configure (WPS / SmartConfig): BL602 has neither; accepted for
    // source compatibility, always fail cleanly (see .cpp).
public:
    bool beginWPSConfig(void);
    bool beginSmartConfig();
    bool stopSmartConfig();
    bool smartConfigDone();

protected:
    static bool _smartConfigStarted;
    static bool _smartConfigDone;
};

#endif /* ESP8266WIFISTA_H_ */
