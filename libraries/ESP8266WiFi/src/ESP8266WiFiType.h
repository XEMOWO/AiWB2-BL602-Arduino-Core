/*
  ESP8266WiFiType.h - esp8266 Wifi types (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino. All right reserved.

  Ported to Ai-WB2-12F (BL602): kept 1:1 with the ESP8266 core so WiFiMode_t,
  WiFiPhyMode_t, WiFiSleepType_t, WiFiEvent_t and WiFiDisconnectReason have the
  exact same enumerators/values third-party ESP8266 code switches on.

  The one deviation: the ESP8266 header pulls in the FreeRTOS <queue.h> for no
  reason; that include is dropped.
 */
#ifndef ESP8266WIFITYPE_H_
#define ESP8266WIFITYPE_H_

#include <stdint.h>
#include "Arduino.h"   /* String, IPAddress */
#include "IPAddress.h"

#define WIFI_SCAN_RUNNING   (-1)
#define WIFI_SCAN_FAILED    (-2)

// Note: these enums need to be in sync with the SDK!
typedef enum WiFiMode
{
    WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3
} WiFiMode_t;

typedef enum WiFiPhyMode
{
    WIFI_PHY_MODE_11B = 1, WIFI_PHY_MODE_11G = 2, WIFI_PHY_MODE_11N = 3
} WiFiPhyMode_t;

typedef enum WiFiSleepType
{
    WIFI_NONE_SLEEP = 0, WIFI_LIGHT_SLEEP = 1, WIFI_MODEM_SLEEP = 2
} WiFiSleepType_t;

// ESP32 compatibility
typedef enum wifi_ps_type
{
    WIFI_PS_NONE = WIFI_NONE_SLEEP,
    WIFI_PS_MIN_MODEM = WIFI_MODEM_SLEEP,
    WIFI_PS_MAX_MODEM = WIFI_LIGHT_SLEEP,
} wifi_ps_type_t;

typedef enum WiFiEvent
{
    WIFI_EVENT_STAMODE_CONNECTED = 0,
    WIFI_EVENT_STAMODE_DISCONNECTED,
    WIFI_EVENT_STAMODE_AUTHMODE_CHANGE,
    WIFI_EVENT_STAMODE_GOT_IP,
    WIFI_EVENT_STAMODE_DHCP_TIMEOUT,
    WIFI_EVENT_SOFTAPMODE_STACONNECTED,
    WIFI_EVENT_SOFTAPMODE_STADISCONNECTED,
    WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED,
    WIFI_EVENT_MODE_CHANGE,
    WIFI_EVENT_SOFTAPMODE_DISTRIBUTE_STA_IP,
    WIFI_EVENT_MAX,
    WIFI_EVENT_ANY = WIFI_EVENT_MAX,
} WiFiEvent_t;

enum WiFiDisconnectReason
{
    WIFI_DISCONNECT_REASON_UNSPECIFIED              = 1,
    WIFI_DISCONNECT_REASON_AUTH_EXPIRE              = 2,
    WIFI_DISCONNECT_REASON_AUTH_LEAVE               = 3,
    WIFI_DISCONNECT_REASON_ASSOC_EXPIRE             = 4,
    WIFI_DISCONNECT_REASON_ASSOC_TOOMANY            = 5,
    WIFI_DISCONNECT_REASON_NOT_AUTHED               = 6,
    WIFI_DISCONNECT_REASON_NOT_ASSOCED              = 7,
    WIFI_DISCONNECT_REASON_ASSOC_LEAVE              = 8,
    WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED         = 9,
    WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD      = 10,  /* 11h */
    WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD     = 11,  /* 11h */
    WIFI_DISCONNECT_REASON_IE_INVALID               = 13,  /* 11i */
    WIFI_DISCONNECT_REASON_MIC_FAILURE              = 14,  /* 11i */
    WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,  /* 11i */
    WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,  /* 11i */
    WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS       = 17,  /* 11i */
    WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID     = 18,  /* 11i */
    WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID  = 19,  /* 11i */
    WIFI_DISCONNECT_REASON_AKMP_INVALID             = 20,  /* 11i */
    WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION    = 21,  /* 11i */
    WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP       = 22,  /* 11i */
    WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED       = 23,  /* 11i */
    WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED    = 24,  /* 11i */

    WIFI_DISCONNECT_REASON_BEACON_TIMEOUT           = 200,
    WIFI_DISCONNECT_REASON_NO_AP_FOUND              = 201,
    WIFI_DISCONNECT_REASON_AUTH_FAIL                = 202,
    WIFI_DISCONNECT_REASON_ASSOC_FAIL               = 203,
    WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT        = 204,
};

struct WiFiEventModeChange
{
    WiFiMode_t oldMode;
    WiFiMode_t newMode;
};

struct WiFiEventStationModeConnected
{
    String ssid;
    uint8_t bssid[6];
    uint8_t channel;
};

struct WiFiEventStationModeDisconnected
{
    String ssid;
    uint8_t bssid[6];
    WiFiDisconnectReason reason;
};

struct WiFiEventStationModeAuthModeChanged
{
    uint8_t oldMode;
    uint8_t newMode;
};

struct WiFiEventStationModeGotIP
{
    IPAddress ip;
    IPAddress mask;
    IPAddress gw;
};

struct WiFiEventSoftAPModeStationConnected
{
    uint8_t mac[6];
    uint8_t aid;
};

struct WiFiEventSoftAPModeStationDisconnected
{
    uint8_t mac[6];
    uint8_t aid;
};

struct WiFiEventSoftAPModeProbeRequestReceived
{
    int rssi;
    uint8_t mac[6];
};

#endif /* ESP8266WIFITYPE_H_ */
