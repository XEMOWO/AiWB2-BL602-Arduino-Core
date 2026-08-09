/*
 * WiFi.h — ESP32-style WiFi for the Ai-WB2-12F (BL602).
 *
 * Backed by the SDK's wifi_mgmr high-level driver, which runs the
 * 802.11 supplicant in a task and reports connection state through the
 * yloop event loop. This class drives wifi_mgmr and maps SDK states to
 * the Arduino wl_status_t vocabulary.
 *
 * The SDK's wireless headers (wifi_mgmr_ext.h etc) carry extern "C"
 * guards already, so they can be included directly.
 */
#ifndef WiFi_h
#define WiFi_h

#include <Arduino.h>
#include <stdint.h>
#include <IPAddress.h>

#include "WiFiType.h"

/* ---- connection state (ESP32-compatible) ---- */
typedef enum {
    WL_NO_SHIELD        = 255,
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
} wl_status_t;

/* ---- modes (ESP32-compatible) ---- */
typedef enum {
    WIFI_OFF = 0,
    WIFI_STA = 1,
    WIFI_AP  = 2,
    WIFI_AP_STA = 3
} wifi_mode_t;

class WiFiClass
{
public:
    WiFiClass() : _mode(WIFI_OFF), _status(WL_DISCONNECTED), _started(false) {}

    /* Switch the radio between STA / AP / OFF. Returns true on success. */
    bool mode(wifi_mode_t m);

    /* Connect as a station. Returns wl_status_t immediately (async connect,
     * poll status() for progress, same as ESP32). */
    wl_status_t begin(const char *ssid, const char *passphrase = NULL);
    wl_status_t begin(const char *ssid) { return begin(ssid, NULL); }

    void disconnect(void);
    wl_status_t status(void) const;

    /* Station info */
    IPAddress localIP(void);
    IPAddress subnetMask(void);
    IPAddress gatewayIP(void);
    IPAddress dnsIP(void);
    String macAddress(void);
    int32_t  RSSI(void);
    String  SSID(void);

    /* Autoconnect (SDK stores last AP in flash and reconnects on boot).
     * Defaults to enabled; call to switch. */
    void setAutoConnect(bool on);
    void setAutoReconnect(bool on);

    /* Low-level radio on/off (used by begin/disconnect). */
    bool isStarted(void) const { return _started; }

private:
    wifi_mode_t _mode;
    mutable volatile wl_status_t _status;
    mutable volatile bool _started;

    void _ensureStaEnabled(void);
    void _syncStatus(void);

public: /* internal event bridge (called from the yloop filter) */
    void onEvent(int code);
};

extern WiFiClass WiFi;

#endif /* WiFi_h */
