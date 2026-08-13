/*
  ESP8266WiFi.cpp - esp8266 WiFi instance definition (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.

  Ported to Ai-WB2-12F (BL602): the reference defines the global `WiFi` object
  here, after the aggregate class from ESP8266WiFi.h. Everything else (printDiag,
  enableWiFiAtBootTime, backend bring-up) lives in ESP8266WiFiGeneric.cpp.
 */
#include "ESP8266WiFi.h"

ESP8266WiFiClass WiFi;
