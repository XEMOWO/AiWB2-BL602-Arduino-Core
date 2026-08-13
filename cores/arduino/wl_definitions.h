/*
  wl_definitions.h - Arduino WiFi shield status vocabulary (ESP8266-compatible).
  Copyright (c) 2011 Arduino. All right reserved.

  Ported to Ai-WB2-12F (BL602): verbatim copy of the ESP8266 core header so
  that `wl_status_t`, `wl_enc_type` and `wl_tcp_state` have the exact same
  values/enumerators third-party ESP8266 code compares against.

  Values matter: WL_DISCONNECTED is 7 (not 6) on ESP8266, and there is a
  WL_WRONG_PASSWORD (=6) that ESP32 does not have. Libraries do
  `WiFi.status() == WL_CONNECTED` and print the numeric status, so this must
  match the reference bit-for-bit.

  NOTE: the ESP8266 core includes this from inside `extern "C" { }`. We do not:
  the file ends with a C++ `using wl_tcp_state = tcp_state;` alias, and the
  SDK lwIP headers already compile from C++ translation units, so a plain
  include is correct and avoids the linkage-specification edge case.
 */
#ifndef WL_DEFINITIONS_H_
#define WL_DEFINITIONS_H_

// Maximum size of a SSID
#define WL_SSID_MAX_LENGTH 32
// Length of passphrase. Valid lengths are 8-63.
#define WL_WPA_KEY_MAX_LENGTH 63
// Length of key in bytes. Valid values are 5 and 13.
#define WL_WEP_KEY_MAX_LENGTH 13
// Size of a MAC-address or BSSID
#define WL_MAC_ADDR_LENGTH 6
// Size of a MAC-address or BSSID
#define WL_IPV4_LENGTH 4
// Maximum size of a SSID list
#define WL_NETWORKS_LIST_MAXNUM 10
// Maxmium number of socket
#define MAX_SOCK_NUM 4
// Socket not available constant
#define SOCK_NOT_AVAIL 255
// Default state value for Wifi state field
#define NA_STATE -1
//Maximum number of attempts to establish wifi connection
#define WL_MAX_ATTEMPT_CONNECTION 10

typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_WRONG_PASSWORD   = 6,
    WL_DISCONNECTED     = 7
} wl_status_t;

/* Encryption modes */
enum wl_enc_type {  /* Values map to 802.11 encryption suites... */
    ENC_TYPE_WEP  = 5,
    ENC_TYPE_TKIP = 2,
    ENC_TYPE_CCMP = 4,
    /* ... except these two, 7 and 8 are reserved in 802.11-2007 */
    ENC_TYPE_NONE = 7,
    ENC_TYPE_AUTO = 8
};

#include <lwip/init.h>
#include <lwip/tcpbase.h>
using wl_tcp_state = tcp_state;

#endif /* WL_DEFINITIONS_H_ */
