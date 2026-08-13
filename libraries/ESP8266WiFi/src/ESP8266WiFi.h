/*
  ESP8266WiFi.h - esp8266 Wifi support (ESP8266-compatible aggregate).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Modified by Ivan Grokhotkov, December 2014.

  Ported to Ai-WB2-12F (BL602): the `WiFi` object is the multiple-inheritance
  aggregate of Generic + STA + Scan + AP, exactly like the ESP8266 core, so
  name-colliding accessors (SSID/RSSI/BSSID/channel) resolve through the
  `using` declarations below. Backend: SDK wifi_mgmr + lwIP sockets.

  NOTE on TLS: the reference header includes WiFiClientSecure/WiFiServerSecure
  and the BearSSL support headers. Those live in this library too (as Phase-E
  placeholders) and are included here for faithful transitive inclusion.
 */
#ifndef WiFi_h
#define WiFi_h

#include <stdint.h>

#include <wl_definitions.h>

/* ESP8266 non-OS-SDK surface (phy_capture, rst_info, wifi_* wrappers) that
 * third-party libs reach through the WiFi include chain. */
#include "user_interface.h"

#include "IPAddress.h"

#include "ESP8266WiFiType.h"
#include "ESP8266WiFiSTA.h"
#include "ESP8266WiFiAP.h"
#include "ESP8266WiFiScan.h"
#include "ESP8266WiFiGeneric.h"

#include "WiFiClient.h"
#include "WiFiServer.h"
#include "WiFiServerSecure.h"
#include "WiFiClientSecure.h"
#include "WiFiUdp.h"
#include "BearSSLHelpers.h"
#include "CertStoreBearSSL.h"

#ifdef DEBUG_ESP_WIFI
#ifdef DEBUG_ESP_PORT
#define DEBUG_WIFI(fmt, ...) DEBUG_ESP_PORT.printf_P( (PGM_P)PSTR(fmt), ##__VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_WIFI
#define DEBUG_WIFI(...) do { (void)0; } while (0)
#endif

extern "C" void enableWiFiAtBootTime (void) __attribute__((noinline));

class ESP8266WiFiClass : public ESP8266WiFiGenericClass, public ESP8266WiFiSTAClass, public ESP8266WiFiScanClass, public ESP8266WiFiAPClass {
public:
    // workaround same function name with different signature
    using ESP8266WiFiGenericClass::channel;

    using ESP8266WiFiSTAClass::SSID;
    using ESP8266WiFiSTAClass::RSSI;
    using ESP8266WiFiSTAClass::BSSID;
    using ESP8266WiFiSTAClass::BSSIDstr;

    using ESP8266WiFiScanClass::SSID;
    using ESP8266WiFiScanClass::encryptionType;
    using ESP8266WiFiScanClass::RSSI;
    using ESP8266WiFiScanClass::BSSID;
    using ESP8266WiFiScanClass::BSSIDstr;
    using ESP8266WiFiScanClass::channel;
    using ESP8266WiFiScanClass::isHidden;

    void printDiag(Print& dest);

    friend class WiFiClient;
    friend class WiFiServer;
};

extern ESP8266WiFiClass WiFi;

#endif /* WiFi_h */
