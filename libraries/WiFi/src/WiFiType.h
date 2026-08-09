/*
 * WiFiType.h — WiFi event reasons / auth types (ESP32-compatible subset).
 */
#ifndef WiFiType_h
#define WiFiType_h

typedef enum {
    WIFI_AUTH_OPEN         = 0,
    WIFI_AUTH_WEP          = 1,
    WIFI_AUTH_WPA_PSK      = 2,
    WIFI_AUTH_WPA2_PSK     = 3,
    WIFI_AUTH_WPA_WPA2_PSK = 4,
    WIFI_AUTH_WPA2_ENTERPRISE = 5,
    WIFI_AUTH_WPA3_PSK     = 6,
    WIFI_AUTH_MAX          = 7
} wifi_auth_mode_t;

typedef enum {
    WIFI_REASON_UNSPECIFIED              = 1,
    WIFI_REASON_AUTH_EXPIRE              = 2,
    WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,
    WIFI_REASON_NO_AP_FOUND              = 200,
    WIFI_REASON_AUTH_FAIL                = 202,
    WIFI_REASON_ASSOC_FAIL               = 203,
    WIFI_REASON_HANDSHAKE_TIMEOUT        = 204,
    WIFI_REASON_CONNECTION_FAIL          = 205,
    WIFI_REASON_AP_TSF_RESET             = 206
} wifi_err_reason_t;

#endif /* WiFiType_h */
