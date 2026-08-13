/*
  WiFiClientSecureBearSSL.h - esp8266 BearSSL TLS client (ESP8266-compatible).

  In the ESP8266 core this header pulls in the real BearSSL backend classes.
  On the WB2 shim the BearSSL-namespace types are aliases for the plain
  WiFiClientSecure/WiFiServerSecure, so this file just forwards to those.
 */
#ifndef WIFICLIENTSECUREBEARSSL_H_
#define WIFICLIENTSECUREBEARSSL_H_

#include "WiFiClientSecure.h"
#include "WiFiServerSecure.h"

#endif /* WIFICLIENTSECUREBEARSSL_H_ */
