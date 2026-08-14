/*
  ESP8266WiFiGeneric.cpp - esp8266 Wifi generic class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.
  Reworked by Markus Sattler, December 2015.

  Ported to Ai-WB2-12F (BL602): this TU owns the shared wifi_mgmr backend
  bring-up (tcpip_init -> hal_wifi_start_firmware_task -> wifi_mgmr_start_background)
  and the yloop EV_WIFI -> ESP8266 WiFiEvent_* dispatch used by the whole library.

  Event payloads are read back from wifi_mgmr state at event time, because the
  SDK posts minimal payloads (value is 0 for CONNECTED/GOT_IP, the 802.11
  status code for DISCONNECT, an index for AP-station events).
 */
#include "WB2WiFiCommon.h"
#include "ESP8266WiFi.h"     /* aggregate ESP8266WiFiClass (printDiag below) */
#include "WiFiState.h"

#include <list>   /* sCbEventList */

/* ---- shared backend state (declared in WB2WiFiCommon.h) ---- */
wifi_interface_t wb2_sta_iface = NULL;
wifi_interface_t wb2_ap_iface = NULL;
bool wb2_wifi_started = false;
WiFiMode_t wb2_wifi_mode = WIFI_OFF;

static wifi_conf_t s_wifi_conf = {
    .country_code = "CN",
    .channel_nums = 13,
};

static int s_tcpip_started = 0;
static int s_filter_registered = 0;
static int s_bg_started = 0;
static int s_wifi_started = 0;

/* ---- event dispatch ---- */

struct WiFiEventHandlerOpaque
{
    WiFiEventHandlerOpaque(WiFiEvent_t event, std::function<void(WiFiEvent_t, const void*)> handler)
        : mEvent(event)
        , mHandler(handler)
        , mCanExpire(true)
    {
    }

    void operator()(WiFiEvent_t evt, const void *e)
    {
        mHandler(evt, e);
    }

    WiFiEvent_t mEvent;
    std::function<void(WiFiEvent_t, const void*)> mHandler;
    bool mCanExpire;
};

static std::list<WiFiEventHandler> sCbEventList;

static void wb2_dispatch_event(WiFiEvent_t evt, const void *payload)
{
    for (auto it = std::begin(sCbEventList); it != std::end(sCbEventList); ) {
        WiFiEventHandler &handler = *it;
        if (handler->mCanExpire && handler.unique()) {
            /* user dropped the returned handle -> auto-remove (matches ESP8266) */
            it = sCbEventList.erase(it);
        } else {
            if (handler->mEvent == evt || handler->mEvent == WIFI_EVENT_ANY) {
                (*handler)(evt, payload);
            }
            ++it;
        }
    }
}

/* Map an SDK disconnect reason code onto the ESP8266 WiFiDisconnectReason. */
static WiFiDisconnectReason wb2_reason_to_wl(uint16_t reason)
{
    switch (reason) {
    case 0:   return WIFI_DISCONNECT_REASON_UNSPECIFIED;
    case 1:   return WIFI_DISCONNECT_REASON_UNSPECIFIED;
    case 2:   return WIFI_DISCONNECT_REASON_AUTH_EXPIRE;
    case 3:   return WIFI_DISCONNECT_REASON_AUTH_LEAVE;
    case 4:   return WIFI_DISCONNECT_REASON_ASSOC_EXPIRE;
    case 5:   return WIFI_DISCONNECT_REASON_ASSOC_TOOMANY;
    case 6:   return WIFI_DISCONNECT_REASON_NOT_AUTHED;
    case 7:   return WIFI_DISCONNECT_REASON_NOT_ASSOCED;
    case 8:   return WIFI_DISCONNECT_REASON_ASSOC_LEAVE;
    case 9:   return WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED;
    case 10:  return WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD;
    case 11:  return WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD;
    case 13:  return WIFI_DISCONNECT_REASON_IE_INVALID;
    case 14:  return WIFI_DISCONNECT_REASON_MIC_FAILURE;
    case 15:  return WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT;
    case 16:  return WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT;
    case 17:  return WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS;
    case 18:  return WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID;
    case 19:  return WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID;
    case 20:  return WIFI_DISCONNECT_REASON_AKMP_INVALID;
    case 21:  return WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION;
    case 22:  return WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP;
    case 23:  return WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED;
    case 24:  return WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED;
    case 200: return WIFI_DISCONNECT_REASON_BEACON_TIMEOUT;
    case 201: return WIFI_DISCONNECT_REASON_NO_AP_FOUND;
    case 202: return WIFI_DISCONNECT_REASON_AUTH_FAIL;
    case 203: return WIFI_DISCONNECT_REASON_ASSOC_FAIL;
    case 204: return WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT;
    default:  return WIFI_DISCONNECT_REASON_UNSPECIFIED;
    }
}

void wb2_wifi_event_filter(input_event_t *event, void *priv)
{
    (void)priv;

    switch (event->code) {
    case CODE_WIFI_ON_INIT_DONE:
        /* normally started synchronously by wb2_wifi_start(); harmless guard
           for code paths that only get the event (SDK provisioning etc). */
        if (!s_bg_started) {
            wifi_mgmr_start_background(&s_wifi_conf);
            s_bg_started = 1;
        }
        break;

    case CODE_WIFI_ON_CONNECTED: {
        WiFiEventStationModeConnected e;
        wifi_mgmr_sta_connect_ind_stat_info_t st;
        memset(&st, 0, sizeof(st));
        memset(&e.bssid, 0, sizeof(e.bssid));
        e.channel = 0;
        wifi_mgmr_sta_connect_ind_stat_get(&st);
        if (st.ssid[0]) {
            e.ssid = String(st.ssid);
            memcpy(e.bssid, st.bssid, 6);
            e.channel = st.chan_id;
        }
        wb2_dispatch_event(WIFI_EVENT_STAMODE_CONNECTED, &e);
        break;
    }

    case CODE_WIFI_ON_DISCONNECT: {
        WiFiEventStationModeDisconnected e;
        wifi_mgmr_sta_connect_ind_stat_info_t st;
        memset(&st, 0, sizeof(st));
        wifi_mgmr_sta_connect_ind_stat_get(&st);
        memset(&e.bssid, 0, sizeof(e.bssid));
        if (st.ssid[0]) {
            e.ssid = String(st.ssid);
            memcpy(e.bssid, st.bssid, 6);
        }
        uint16_t status_code = 0, reason_code = 0;
        wifi_mgmr_conn_result_get(&status_code, &reason_code);
        e.reason = wb2_reason_to_wl(reason_code);
        wb2_dispatch_event(WIFI_EVENT_STAMODE_DISCONNECTED, &e);
        break;
    }

    case CODE_WIFI_ON_GOT_IP: {
        WiFiEventStationModeGotIP e;
        uint32_t ip = 0, gw = 0, mask = 0;
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
        e.ip   = IPAddress(ip);
        e.mask = IPAddress(mask);
        e.gw   = IPAddress(gw);
        wb2_dispatch_event(WIFI_EVENT_STAMODE_GOT_IP, &e);
        break;
    }

    case CODE_WIFI_ON_AP_STA_ADD: {
        WiFiEventSoftAPModeStationConnected e;
        memset(e.mac, 0, sizeof(e.mac));
        e.aid = (uint8_t)event->value;
        wifi_sta_basic_info_t info;
        if (wifi_mgmr_ap_sta_info_get(&info, e.aid) == 0 && info.is_used) {
            memcpy(e.mac, info.sta_mac, 6);
        }
        wb2_dispatch_event(WIFI_EVENT_SOFTAPMODE_STACONNECTED, &e);
        break;
    }

    case CODE_WIFI_ON_AP_STA_DEL: {
        WiFiEventSoftAPModeStationDisconnected e;
        memset(e.mac, 0, sizeof(e.mac));
        e.aid = (uint8_t)event->value;
        wb2_dispatch_event(WIFI_EVENT_SOFTAPMODE_STADISCONNECTED, &e);
        break;
    }

    default:
        break;
    }
}

/* ---- shared backend bring-up ---- */

void wb2_wifi_start(void)
{
    if (s_wifi_started) {
        return;
    }

    if (!s_tcpip_started) {
        tcpip_init(NULL, NULL);
        s_tcpip_started = 1;
    }

    if (!s_filter_registered) {
        aos_register_event_filter(EV_WIFI, wb2_wifi_event_filter, NULL);
        s_filter_registered = 1;
    }

    hal_wifi_start_firmware_task();

    /* The SDK starts the mgmr inside the yloop INIT_DONE filter. Arduino calls
       WiFi.* from setup()/loop() which run in a DIFFERENT task than the yloop
       loop, so waiting for the event round-trip would race. Start the mgmr task
       synchronously here; the INIT_DONE filter handler is a guarded no-op. */
    if (!s_bg_started) {
        wifi_mgmr_start_background(&s_wifi_conf);
        s_bg_started = 1;
    }

    s_wifi_started = 1;
    wb2_wifi_started = true;
}

wl_status_t wb2_sta_wl_status(void)
{
    if (!wb2_wifi_started) {
        return WL_IDLE_STATUS;
    }
    int sdk_state = 0;
    wifi_mgmr_sta_state_get(&sdk_state);
    switch (sdk_state) {
    case WIFI_STATE_CONNECTED_IP_GOT:
    case WIFI_STATE_WITH_AP_CONNECTED_IP_GOT:
        return WL_CONNECTED;
    case WIFI_STATE_PSK_ERROR:
        return WL_WRONG_PASSWORD;
    case WIFI_STATE_NO_AP_FOUND:
        return WL_NO_SSID_AVAIL;
    case WIFI_STATE_CONNECTING:
    case WIFI_STATE_WITH_AP_CONNECTING:
    case WIFI_STATE_CONNECTED_IP_GETTING:
    case WIFI_STATE_WITH_AP_CONNECTED_IP_GETTING:
        return WL_IDLE_STATUS;
    case WIFI_STATE_DISCONNECT:
    case WIFI_STATE_WITH_AP_DISCONNECT:
    default:
        return WL_DISCONNECTED;
    }
}

/* ---- ESP8266WiFiGenericClass ---- */

bool ESP8266WiFiGenericClass::_persistent = false;
WiFiMode_t ESP8266WiFiGenericClass::_forceSleepLastMode = WIFI_OFF;

ESP8266WiFiGenericClass::ESP8266WiFiGenericClass()
{
    /* the yloop filter is registered lazily in wb2_wifi_start() */
}

void ESP8266WiFiGenericClass::onEvent(WiFiEventCb f, WiFiEvent_t event)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        event, [f](WiFiEvent_t evt, const void *e) {
            (void)e;
            (*f)(evt);
        });
    handler->mCanExpire = false;   /* C callbacks are permanent, like ESP8266 */
    sCbEventList.push_back(handler);
}

WiFiEventHandler ESP8266WiFiGenericClass::onStationModeConnected(std::function<void(const WiFiEventStationModeConnected&)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_STAMODE_CONNECTED, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventStationModeConnected*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onStationModeDisconnected(std::function<void(const WiFiEventStationModeDisconnected&)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_STAMODE_DISCONNECTED, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventStationModeDisconnected*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onStationModeAuthModeChanged(std::function<void(const WiFiEventStationModeAuthModeChanged&)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_STAMODE_AUTHMODE_CHANGE, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventStationModeAuthModeChanged*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onStationModeGotIP(std::function<void(const WiFiEventStationModeGotIP&)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_STAMODE_GOT_IP, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventStationModeGotIP*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onStationModeDHCPTimeout(std::function<void(void)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_STAMODE_DHCP_TIMEOUT, [f](WiFiEvent_t, const void *e) {
            (void)e;
            f();
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onSoftAPModeStationConnected(std::function<void(const WiFiEventSoftAPModeStationConnected&)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_SOFTAPMODE_STACONNECTED, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventSoftAPModeStationConnected*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onSoftAPModeStationDisconnected(std::function<void(const WiFiEventSoftAPModeStationDisconnected&)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_SOFTAPMODE_STADISCONNECTED, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventSoftAPModeStationDisconnected*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onSoftAPModeProbeRequestReceived(std::function<void(const WiFiEventSoftAPModeProbeRequestReceived&)> f)
{
    /* BL602 firmware does not expose probe-request notifications through yloop;
       the event is accepted but never delivered. */
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventSoftAPModeProbeRequestReceived*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

WiFiEventHandler ESP8266WiFiGenericClass::onWiFiModeChange(std::function<void(const WiFiEventModeChange&)> f)
{
    WiFiEventHandler handler = std::make_shared<WiFiEventHandlerOpaque>(
        WIFI_EVENT_MODE_CHANGE, [f](WiFiEvent_t, const void *e) {
            f(*static_cast<const WiFiEventModeChange*>(e));
        });
    sCbEventList.push_back(handler);
    return handler;
}

uint8_t ESP8266WiFiGenericClass::channel(void)
{
    if (!wb2_wifi_started) {
        return 0;
    }
    int ch = 0;
    wifi_mgmr_channel_get(&ch);
    return (uint8_t)ch;
}

bool ESP8266WiFiGenericClass::setSleepMode(WiFiSleepType_t type, uint8_t listenInterval)
{
    (void)listenInterval;
    if (type == WIFI_MODEM_SLEEP && wb2_wifi_started) {
        wifi_mgmr_sta_ps_enter(0);   /* ps_level 0: modem sleep */
    } else if (type == WIFI_NONE_SLEEP && wb2_wifi_started) {
        wifi_mgmr_sta_ps_exit();
    }
    return true;
}

WiFiSleepType_t ESP8266WiFiGenericClass::getSleepMode(void)
{
    return WIFI_MODEM_SLEEP;
}

uint8_t ESP8266WiFiGenericClass::getListenInterval(void)
{
    return 0;
}

bool ESP8266WiFiGenericClass::isSleepLevelMax(void)
{
    return false;
}

bool ESP8266WiFiGenericClass::setPhyMode(WiFiPhyMode_t mode)
{
    (void)mode;
    /* BL602 firmware picks its own PHY rate; accept the request silently. */
    return true;
}

WiFiPhyMode_t ESP8266WiFiGenericClass::getPhyMode(void)
{
    return WIFI_PHY_MODE_11N;
}

void ESP8266WiFiGenericClass::setOutputPower(float dBm)
{
    (void)dBm;
    /* not controllable through wifi_mgmr on BL602 */
}

void ESP8266WiFiGenericClass::persistent(bool persistent)
{
    _persistent = persistent;
}

bool ESP8266WiFiGenericClass::getPersistent()
{
    return _persistent;
}

bool ESP8266WiFiGenericClass::mode(WiFiMode_t m)
{
    if (!wb2_wifi_started && m != WIFI_OFF) {
        wb2_wifi_start();
    }

    switch (m) {
    case WIFI_STA:
        wb2_sta_iface = wifi_mgmr_sta_enable();
        break;
    case WIFI_AP:
        wb2_ap_iface = wifi_mgmr_ap_enable();
        break;
    case WIFI_AP_STA:
        wb2_sta_iface = wifi_mgmr_sta_enable();
        wb2_ap_iface = wifi_mgmr_ap_enable();
        break;
    case WIFI_OFF:
    default:
        if (wb2_sta_iface) {
            wifi_mgmr_sta_disable(&wb2_sta_iface);
            wb2_sta_iface = NULL;
        }
        if (wb2_ap_iface) {
            wifi_mgmr_ap_stop(&wb2_ap_iface);
            wb2_ap_iface = NULL;
        }
        break;
    }

    WiFiEventModeChange ev;
    ev.oldMode = wb2_wifi_mode;
    ev.newMode = m;
    wb2_wifi_mode = m;
    wb2_dispatch_event(WIFI_EVENT_MODE_CHANGE, &ev);
    return true;
}

WiFiMode_t ESP8266WiFiGenericClass::getMode(void)
{
    return wb2_wifi_mode;
}

bool ESP8266WiFiGenericClass::enableSTA(bool enable)
{
    if (enable) {
        return mode(WIFI_STA);
    }
    if (wb2_wifi_mode == WIFI_AP_STA) {
        return mode(WIFI_AP);
    }
    return mode(WIFI_OFF);
}

bool ESP8266WiFiGenericClass::enableAP(bool enable)
{
    if (enable) {
        return mode(WIFI_AP);
    }
    if (wb2_wifi_mode == WIFI_AP_STA) {
        return mode(WIFI_STA);
    }
    return mode(WIFI_OFF);
}

bool ESP8266WiFiGenericClass::forceSleepBegin(uint32_t sleepUs)
{
    (void)sleepUs;
    return false;   /* BL602 has no ESP8266-style forced radio sleep */
}

bool ESP8266WiFiGenericClass::forceSleepWake(void)
{
    return false;
}

bool ESP8266WiFiGenericClass::shutdown(WiFiState &stateSave)
{
    stateSave.state.mode = wb2_wifi_mode;
    return true;
}

bool ESP8266WiFiGenericClass::shutdown(WiFiState &stateSave, uint32_t sleepUs)
{
    (void)sleepUs;
    return shutdown(stateSave);
}

bool ESP8266WiFiGenericClass::resumeFromShutdown(WiFiState &savedState)
{
    mode(savedState.state.mode);
    return true;
}

bool ESP8266WiFiGenericClass::shutdownValidCRC(const WiFiState &state)
{
    (void)state;
    return true;
}

void ESP8266WiFiGenericClass::preinitWiFiOff(void)
{
    /* wifi_mgmr does not auto-start on the Arduino core; nothing to preinit. */
}

int ESP8266WiFiGenericClass::hostByName(const char* aHostname, IPAddress& aResult)
{
    return hostByName(aHostname, aResult, DNSDefaultTimeoutMs, DNSResolveTypeDefault);
}

int ESP8266WiFiGenericClass::hostByName(const char* aHostname, IPAddress& aResult, uint32_t timeout_ms)
{
    return hostByName(aHostname, aResult, timeout_ms, DNSResolveTypeDefault);
}

int ESP8266WiFiGenericClass::hostByName(const char* aHostname, IPAddress& aResult, uint32_t timeout_ms, DNSResolveType resolveType)
{
    (void)timeout_ms;
    (void)resolveType;

    if (!aHostname || !*aHostname) {
        return 0;
    }

    /* dotted-quad literal: no DNS round-trip needed */
    unsigned a, b, c, d;
    if (sscanf(aHostname, "%u.%u.%u.%u", &a, &b, &c, &d) == 4 &&
        a < 256 && b < 256 && c < 256 && d < 256) {
        uint8_t *p = aResult.raw_address();
        p[0] = (uint8_t)a; p[1] = (uint8_t)b; p[2] = (uint8_t)c; p[3] = (uint8_t)d;
        return 1;
    }

    if (!wb2_wifi_started) {
        return 0;
    }

    /* blocking resolver (lwIP dns_gethostbyname wrapper) */
    struct hostent *he = lwip_gethostbyname(aHostname);
    if (!he || !he->h_addr_list || !he->h_addr_list[0]) {
        return 0;
    }
    memcpy(aResult.raw_address(), he->h_addr_list[0], 4);
    return 1;
}

void ESP8266WiFiClass::printDiag(Print& dest)
{
    dest.printf("Mode: %d\r\n", (int)getMode());
    dest.printf("Status: %d\r\n", (int)status());
    if (getMode() & WIFI_STA) {
        dest.printf("SSID: %s\r\n", SSID().c_str());
        dest.printf("IP: %s\r\n", localIP().toString().c_str());
        dest.printf("Mask: %s\r\n", subnetMask().toString().c_str());
        dest.printf("GW: %s\r\n", gatewayIP().toString().c_str());
        dest.printf("DNS: %s\r\n", dnsIP().toString().c_str());
        dest.printf("RSSI: %d\r\n", (int)RSSI());
    }
    if (getMode() & WIFI_AP) {
        dest.printf("AP IP: %s\r\n", softAPIP().toString().c_str());
        dest.printf("AP MAC: %s\r\n", softAPmacAddress().c_str());
    }
}

/* Legacy nonos-SDK entry point: make sure the radio is up so the stored-AP
 * autoconnect inside wifi_mgmr can proceed without an explicit begin(). */
extern "C" void enableWiFiAtBootTime (void) __attribute__((noinline));
extern "C" void enableWiFiAtBootTime (void)
{
    wb2_wifi_start();
    if (wb2_wifi_mode == WIFI_OFF) {
        wb2_wifi_mode = WIFI_STA;
        wb2_sta_iface = wifi_mgmr_sta_enable();
    }
    wifi_mgmr_sta_autoconnect_enable();
}

/* ---- STA hostname (ESP8266 LwipIntf API, mirrored on the Generic class) ----
 *
 * IPv6.ino calls WiFi.hostname("ipv6test"); AddrList then reports the name
 * through each netif's hostname field. lwIP netifs keep only a *pointer* to
 * the name, so it must point at persistent storage — hence the static String.
 * The SDK's default STA hostname is "espressif" (ESP8266's default).
 */
static String s_hostname;

/* Effective hostname, with the SDK default materialized on first use.
 *
 * This must stay lazy: a file-scope `static String s_hostname = "espressif"`
 * allocates the buffer in .init_array, which BL602 runs BEFORE
 * vPortDefineHeapRegions() (newlib malloc == pvPortMalloc here, and a malloc
 * before the FreeRTOS heap exists hits the SDK's ecall trap -> reset). A plain
 * default-constructed String performs no allocation, so the constructor is
 * harmless; only the first read/write (all post-boot) touches the heap.
 */
static const char* s_hostname_effective()
{
    if (s_hostname.length() == 0) {
        s_hostname = "espressif";
    }
    return s_hostname.c_str();
}

String ESP8266WiFiGenericClass::hostname()
{
    return String(s_hostname_effective());
}

bool ESP8266WiFiGenericClass::hostname(const char* aHostname)
{
    if (!aHostname || !*aHostname) {
        return false;
    }
    s_hostname = aHostname;
    /* Point every lwIP netif at our storage (netif holds only a pointer). */
    for (struct netif* nif = netif_list; nif != NULL; nif = nif->next) {
        netif_set_hostname(nif, s_hostname.c_str());
    }
    return true;
}

const char* ESP8266WiFiGenericClass::getHostname()
{
    return s_hostname_effective();
}

/* non-OS-SDK: CallSDKFunctions and a few libraries read this directly. */
extern "C" const char* wifi_station_get_hostname(void)
{
    return s_hostname_effective();
}

/* ---- non-OS-SDK wifi control (declared in cores/arduino/user_interface.h)
 *
 * TestEspApi / interactive drive the radio through the old non-OS-SDK calls.
 * The opmode and sleep-type enums happen to share numeric values with our
 * WiFiMode_t / WiFiSleepType_t, so these map straight onto the class API.
 * Event delivery (wifi_set_event_handler_cb) is accepted and stored, but the
 * BL602 yloop event pump already dispatches to the ESP8266 WiFiEvent_* lambdas,
 * so the legacy callback is not re-raised.
 */
static wifi_event_handler_cb_t s_legacy_event_cb = NULL;

extern "C" uint8 wifi_get_opmode(void)
{
    return (uint8)wb2_wifi_mode;
}

extern "C" uint8 wifi_get_opmode_default(void)
{
    return (uint8)wb2_wifi_mode;
}

extern "C" bool wifi_set_opmode(uint8 opmode)
{
    if (opmode > 3) {
        return false;
    }
    return WiFi.mode((WiFiMode_t)opmode);
}

extern "C" uint8 wifi_get_broadcast_if(void)
{
    return (wb2_wifi_mode & WIFI_STA) ? STATION_IF : SOFTAP_IF;
}

/* Non-OS-SDK STA link control (interactive.ino 'c'/'C'): a no-arg WiFi.begin()
 * connects with the stored credentials, WiFi.disconnect(false) drops the link
 * while keeping the config. */
extern "C" bool wifi_station_connect(void)
{
    WiFi.begin();
    return true;
}

extern "C" bool wifi_station_disconnect(void)
{
    return WiFi.disconnect(false);
}

extern "C" uint8 wifi_get_channel(void)
{
    return WiFi.channel();
}

extern "C" bool wifi_set_channel(uint8 ch)
{
    (void)ch; /* SDK has no public channel-set; accepted for compatibility */
    return true;
}

extern "C" bool wifi_get_country(wifi_country_t *country)
{
    if (!country) return false;
    /* ESP8266 default country; BL602 has no regulator state to report. */
    country->cc[0] = 'C'; country->cc[1] = 'N'; country->cc[2] = '\0';
    country->schan = 1;
    country->nchan = 13;
    country->policy = WIFI_COUNTRY_POLICY_AUTO;
    return true;
}

extern "C" bool wifi_set_country(wifi_country_t *country)
{
    (void)country; /* accepted for compatibility; nothing to configure */
    return true;
}

extern "C" uint8 wifi_get_phy_mode(void)
{
    return (uint8)WIFI_PHY_MODE_11G;
}

extern "C" bool wifi_set_phy_mode(uint8 mode)
{
    (void)mode; /* fixed 11G/11N-capable radio; accepted */
    return true;
}

extern "C" bool wifi_set_sleep_type(sleep_type_t type)
{
    if (type > MODEM_SLEEP_T) {
        return false;
    }
    return WiFi.setSleepMode((WiFiSleepType_t)type);
}

extern "C" void wifi_set_event_handler_cb(wifi_event_handler_cb_t cb)
{
    s_legacy_event_cb = cb; /* stored; not re-raised (see header note) */
}

/* FreeRTOS-like modem power control: the BL602 SDK owns radio power
 * management, so these are accepted and left alone. */
extern "C" void wifi_fpm_open(void)      { /* no-op */ }
extern "C" void wifi_fpm_close(void)     { /* no-op */ }
extern "C" void wifi_fpm_do_wakeup(void) { /* no-op */ }
extern "C" void wifi_fpm_set_sleep_type(sleep_type_t type) { (void)type; }

extern "C" sint8 wifi_fpm_do_sleep(uint32 sleep_time_in_us)
{
    (void)sleep_time_in_us;
    return 0; /* sleep accepted, no modem power-down on BL602 */
}

extern "C" void wifi_fpm_set_wakeup_cb(fpm_wakeup_cb_t cb) { (void)cb; }

/* LowPowerDemo GPIO-wakeup API. BL602 has no fast-power-mode wakeup via this
 * SDK, so these compile and are otherwise inert. */
extern "C" void gpio_pin_wakeup_enable(uint32_t i, GPIO_INT_TYPE intr_state)
{
    (void)i; (void)intr_state;
}
extern "C" void gpio_pin_wakeup_disable(uint32_t i) { (void)i; }
