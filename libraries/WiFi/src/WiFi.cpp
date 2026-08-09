/*
 * WiFi.cpp — ESP32-style WiFi for the Ai-WB2-12F (BL602).
 *
 * Initialisation follows the SDK's station example:
 *   1. tcpip_init() once (lwIP)
 *   2. wifi_mgmr_start_background(&conf) -> wifi_mgmr_drv_init (starts the
 *      M0 firmware task) + a wifi_mgmr task
 *   3. wifi_mgmr_sta_enable() + wifi_mgmr_sta_connect()
 *
 * Connection progress arrives on the yloop EV_WIFI event channel; this
 * class tracks it into an Arduino wl_status_t.
 *
 * The SDK's wireless headers carry extern "C" guards already.
 */
#include "WiFi.h"

#include <string.h>
#include <aos/yloop.h>
#include <lwip/tcpip.h>
#include <lwip/init.h>
#include <hal_wifi.h>
#include <wifi_mgmr_ext.h>

WiFiClass WiFi;

/* shared wifi config: country code defaults to CN */
static wifi_conf_t s_wifi_conf = {
    .country_code = "CN",
};

static int s_tcpip_started = 0;
static int s_wifi_started = 0;
static char s_connected_ssid[32] = "";

static wl_status_t sdk_state_to_wl(int sdk_state)
{
    switch (sdk_state) {
    case WIFI_STATE_CONNECTED_IP_GOT:
    case WIFI_STATE_WITH_AP_CONNECTED_IP_GOT:
        return WL_CONNECTED;
    case WIFI_STATE_CONNECTING:
    case WIFI_STATE_WITH_AP_CONNECTING:
    case WIFI_STATE_CONNECTED_IP_GETTING:
    case WIFI_STATE_WITH_AP_CONNECTED_IP_GETTING:
        return WL_IDLE_STATUS;
    case WIFI_STATE_PSK_ERROR:
        return WL_CONNECT_FAILED;
    case WIFI_STATE_NO_AP_FOUND:
        return WL_NO_SSID_AVAIL;
    case WIFI_STATE_DISCONNECT:
    case WIFI_STATE_WITH_AP_DISCONNECT:
    default:
        return WL_DISCONNECTED;
    }
}

static void wifi_event_cb(input_event_t *event, void *priv)
{
    (void)priv;
    if (event->code == CODE_WIFI_ON_INIT_DONE) {
        /* firmware task is up; start the wifi_mgmr background manager */
        wifi_mgmr_start_background(&s_wifi_conf);
    } else if (event->code == CODE_WIFI_ON_PROV_SSID) {
        /* firmware echoes the SSID it connected to; cache it for SSID() */
        if (event->value) {
            strncpy(s_connected_ssid, (const char *)event->value, sizeof(s_connected_ssid) - 1);
            s_connected_ssid[sizeof(s_connected_ssid) - 1] = 0;
        }
    }
    WiFi.onEvent(event->code);
}

/* ------------------------------------------------------------------ */

void WiFiClass::_ensureStaEnabled(void)
{
    if (_mode == WIFI_STA || _mode == WIFI_AP_STA) {
        return;
    }
    _mode = WIFI_STA;
}

wl_status_t WiFiClass::begin(const char *ssid, const char *passphrase)
{
    wifi_interface_t iface;

    if (!ssid || !ssid[0]) {
        return WL_CONNECT_FAILED;
    }

    if (!s_tcpip_started) {
        tcpip_init(NULL, NULL);
        s_tcpip_started = 1;
    }

    _ensureStaEnabled();
    _status = WL_IDLE_STATUS;
    _started = true;

    /* Start the radio + mgmr if this is the first begin(). */
    if (!s_wifi_started) {
        static int filter_registered = 0;
        if (!filter_registered) {
            aos_register_event_filter(EV_WIFI, wifi_event_cb, NULL);
            filter_registered = 1;
        }
        hal_wifi_start_firmware_task();
        aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
        s_wifi_started = 1;
    }

    iface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(&iface, (char *)ssid, (char *)(passphrase ? passphrase : ""),
                          NULL, NULL, 0, 0);
    strncpy(s_connected_ssid, ssid, sizeof(s_connected_ssid) - 1);
    s_connected_ssid[sizeof(s_connected_ssid) - 1] = 0;

    return WL_IDLE_STATUS;
}

void WiFiClass::disconnect(void)
{
    if (_started) {
        wifi_mgmr_sta_disconnect();
        _status = WL_DISCONNECTED;
    }
}

bool WiFiClass::mode(wifi_mode_t m)
{
    _mode = m;
    return true;
}

wl_status_t WiFiClass::status(void) const
{
    if (_started) {
        int sdk_state = 0;
        wifi_mgmr_state_get(&sdk_state);
        _status = sdk_state_to_wl(sdk_state);
    }
    return _status;
}

void WiFiClass::_syncStatus(void)
{
    int sdk_state = 0;
    wifi_mgmr_state_get(&sdk_state);
    _status = sdk_state_to_wl(sdk_state);
}

void WiFiClass::onEvent(int code)
{
    switch (code) {
    case CODE_WIFI_ON_CONNECTED:
        _status = WL_IDLE_STATUS; /* IP not assigned yet */
        _started = true;
        break;
    case CODE_WIFI_ON_DISCONNECT:
        _status = WL_DISCONNECTED;
        break;
    case CODE_WIFI_ON_GOT_IP:
        _status = WL_CONNECTED;
        break;
    default:
        break;
    }
}

IPAddress WiFiClass::localIP(void)
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (_started) {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(ip);
}

IPAddress WiFiClass::subnetMask(void)
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (_started) {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(mask);
}

IPAddress WiFiClass::gatewayIP(void)
{
    uint32_t ip = 0, gw = 0, mask = 0;
    if (_started) {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }
    return IPAddress(gw);
}

IPAddress WiFiClass::dnsIP(void)
{
    uint32_t dns1 = 0, dns2 = 0;
    if (_started) {
        wifi_mgmr_sta_dns_get(&dns1, &dns2);
    }
    return IPAddress(dns1);
}

String WiFiClass::macAddress(void)
{
    uint8_t mac[6];
    char buf[18];
    if (_started) {
        wifi_mgmr_sta_mac_get(mac);
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return String(buf);
    }
    return String("00:00:00:00:00:00");
}

int32_t WiFiClass::RSSI(void)
{
    int rssi = 0;
    if (_started) {
        wifi_mgmr_rssi_get(&rssi);
    }
    return rssi;
}

String WiFiClass::SSID(void)
{
    return String(s_connected_ssid);
}

void WiFiClass::setAutoConnect(bool on)
{
    if (on) {
        wifi_mgmr_sta_autoconnect_enable();
    } else {
        wifi_mgmr_sta_autoconnect_disable();
    }
}

void WiFiClass::setAutoReconnect(bool on)
{
    /* autoconnect in this SDK covers reconnect-after-dropped too */
    setAutoConnect(on);
}
