/*
  ESP8266WiFiGeneric.h - esp8266 Wifi generic class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): API kept 1:1 with the ESP8266 core. The
  SDK's wifi_mgmr (bl60x_wifi_driver) is the backend; SDK yloop EV_WIFI events
  are mapped to the ESP8266 WiFiEvent_t vocabulary in ESP8266WiFiGeneric.cpp.

  Deviations from the reference (all documented):
    - hostByName() uses lwIP's blocking gethostbyname(); the timeout_ms and
      DNSResolveType variants accept the args for source compatibility but
      resolve via the same blocking call.
    - forceSleepBegin/Wake/shutdown/resumeFromShutdown are safe no-ops:
      BL602 has no ESP8266-style modem deep sleep and the SDK owns power
      management. persistent() is a stored flag only.
 */
#ifndef ESP8266WIFIGENERIC_H_
#define ESP8266WIFIGENERIC_H_

#include "ESP8266WiFiType.h"

#include <IPAddress.h>

#include <functional>
#include <memory>

#ifdef DEBUG_ESP_WIFI
#ifdef DEBUG_ESP_PORT
#define DEBUG_WIFI_GENERIC(fmt, ...) DEBUG_ESP_PORT.printf_P( (PGM_P)PSTR(fmt), ##__VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_WIFI_GENERIC
#define DEBUG_WIFI_GENERIC(...) do { (void)0; } while (0)
#endif

struct WiFiEventHandlerOpaque;
typedef std::shared_ptr<WiFiEventHandlerOpaque> WiFiEventHandler;

typedef void (*WiFiEventCb)(WiFiEvent_t);

/* DNS address-type selector. Values mirror lwIP's LWIP_DNS_ADDRTYPE_* so code
 * that casts between the two keeps working. */
enum class DNSResolveType : uint8_t
{
    DNS_AddrType_IPv4 = 0,
    DNS_AddrType_IPv6 = 1,
    DNS_AddrType_IPv4_IPv6 = 2,
    DNS_AddrType_IPv6_IPv4 = 3,
};

inline constexpr auto DNSDefaultTimeoutMs = 10000;
inline constexpr auto DNSResolveTypeDefault = static_cast<DNSResolveType>(0);

struct WiFiState;

class ESP8266WiFiGenericClass {
public:
    ESP8266WiFiGenericClass();

    // Note: this function is deprecated. Use one of the functions below instead.
    void onEvent(WiFiEventCb cb, WiFiEvent_t event = WIFI_EVENT_ANY) __attribute__((deprecated));

    [[nodiscard]] WiFiEventHandler onStationModeConnected(std::function<void(const WiFiEventStationModeConnected&)>);
    [[nodiscard]] WiFiEventHandler onStationModeDisconnected(std::function<void(const WiFiEventStationModeDisconnected&)>);
    [[nodiscard]] WiFiEventHandler onStationModeAuthModeChanged(std::function<void(const WiFiEventStationModeAuthModeChanged&)>);
    [[nodiscard]] WiFiEventHandler onStationModeGotIP(std::function<void(const WiFiEventStationModeGotIP&)>);
    [[nodiscard]] WiFiEventHandler onStationModeDHCPTimeout(std::function<void(void)>);
    [[nodiscard]] WiFiEventHandler onSoftAPModeStationConnected(std::function<void(const WiFiEventSoftAPModeStationConnected&)>);
    [[nodiscard]] WiFiEventHandler onSoftAPModeStationDisconnected(std::function<void(const WiFiEventSoftAPModeStationDisconnected&)>);
    [[nodiscard]] WiFiEventHandler onSoftAPModeProbeRequestReceived(std::function<void(const WiFiEventSoftAPModeProbeRequestReceived&)>);
    [[nodiscard]] WiFiEventHandler onWiFiModeChange(std::function<void(const WiFiEventModeChange&)>);

    uint8_t channel(void);

    bool setSleepMode(WiFiSleepType_t type, uint8_t listenInterval = 0);
    bool setSleep(bool enable)
    {
        if (enable) {
            return setSleepMode(WIFI_MODEM_SLEEP);
        } else {
            return setSleepMode(WIFI_NONE_SLEEP);
        }
    }
    bool setSleep(wifi_ps_type_t mode)
    {
        return setSleepMode((WiFiSleepType_t)mode);
    }
    bool getSleep()
    {
        return getSleepMode() == WIFI_MODEM_SLEEP;
    }

    WiFiSleepType_t getSleepMode();
    uint8_t getListenInterval();
    bool isSleepLevelMax();

    bool setPhyMode(WiFiPhyMode_t mode);
    WiFiPhyMode_t getPhyMode();

    void setOutputPower(float dBm);

    static void persistent(bool persistent);
    static bool getPersistent();

    bool mode(WiFiMode_t);
    WiFiMode_t getMode();

    bool enableSTA(bool enable);
    bool enableAP(bool enable);

    bool forceSleepBegin(uint32_t sleepUs = 0);
    bool forceSleepWake();

    // wrappers around mode() and forceSleepBegin/Wake; the WiFiState hold is
    // accepted for source compatibility (BL602 has no ESP8266-style save/restore
    // of the radio config, so state is not preserved across these calls).
    bool shutdown(WiFiState& stateSave);
    bool shutdown(WiFiState& stateSave, uint32_t sleepUs);
    bool resumeFromShutdown(WiFiState& savedState);

    static bool shutdownValidCRC (const WiFiState& state);
    static void preinitWiFiOff() __attribute__((deprecated("WiFi is off by default at boot, use enableWiFiAtBoot() for legacy behavior")));

protected:
    static bool _persistent;
    static WiFiMode_t _forceSleepLastMode;

    // -------------------------------------------------------------------------
    // Generic Network functions
    // -------------------------------------------------------------------------
public:
    int hostByName(const char* aHostname, IPAddress& aResult);
    int hostByName(const char* aHostname, IPAddress& aResult, uint32_t timeout_ms);
    int hostByName(const char* aHostname, IPAddress& aResult, uint32_t timeout_ms, DNSResolveType resolveType);

    /* ESP8266's LwipIntf base puts the STA hostname API here (the reference
     * core inlines the LwipIntf helpers into this class). IPv6.ino calls
     * WiFi.hostname("ipv6test"); CallSDKFunctions uses wifi_station_get_hostname(). */
    String hostname();
    bool hostname(const String& aHostname)
    {
        return hostname(aHostname.c_str());
    }
    bool hostname(const char* aHostname);
    bool setHostname(const char* aHostName)
    {
        return hostname(aHostName);
    }
    const char* getHostname();

protected:
    friend class ESP8266WiFiSTAClass;
    friend class ESP8266WiFiScanClass;
    friend class ESP8266WiFiAPClass;
};

#endif /* ESP8266WIFIGENERIC_H_ */
