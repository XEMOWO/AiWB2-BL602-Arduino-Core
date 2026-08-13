/*
 * user_interface.h — ESP8266-compatible non-OS-SDK string helpers.
 *
 * On ESP8266 this header (tools/sdk/include/user_interface.h) provides the
 * os_str* macros that third-party libraries (e.g. ESP8266mDNS) use as aliases
 * for the libc functions. BL602 has no non-OS SDK layer, so these map straight
 * to the toolchain's libc — same observable behavior.
 */
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <string.h>
#include <stdint.h>
/* struct timeval / gettimeofday — the core implements gettimeofday (time.cpp);
 * pulled in here so code that reaches this SDK-compat header (Netdump via the
 * WiFi include chain) can call it without an explicit <sys/time.h>. */
#include <sys/time.h>
/* ip_addr_t for struct ip_info (SDK lwIP, IPv4-only build) */
#include <lwip/ip_addr.h>
/* STAILQ_ENTRY / STAILQ_NEXT for the non-OS-SDK connected-station list
 * (StaticLease example walks it). The toolchain's newlib ships sys/queue.h. */
#include <sys/queue.h>
/* SDK type spellings (uint8/uint16/uint32, ICACHE_FLASH_ATTR, ...) — the
 * ESP8266 non-OS SDK's c_types.h, shipped verbatim. SoftwareSerial and other
 * ports include it directly. */
#include <c_types.h>

#define os_strcmp  strcmp
#define os_strncmp strncmp
#define os_strlen  strlen
#define os_strcpy  strcpy
#define os_strncpy strncpy
#define os_strcat  strcat
#define os_strchr  strchr
#define os_strstr  strstr
#define os_strrchr strrchr
#define os_strtok  strtok
#define os_memcpy  memcpy
#define os_memcmp  memcmp
#define os_memset  memset
#define os_memmove memmove
#define os_sprintf sprintf
#define os_snprintf snprintf

/* Reset-cause plumbing from the ESP8266 non-OS SDK (user_interface.h).
 * DNSServer and a few libs reference `struct rst_info resetInfo` and the
 * REASON_* enum. Values match the ESP8266 SDK. The `resetInfo` global is
 * defined in Esp.cpp (BL602 reset-reason backend). */
struct rst_info {
    uint32_t reason;   /* rst_reason */
    uint32_t exccause;
    uint32_t epc1;
    uint32_t epc2;
    uint32_t epc3;
    uint32_t excvaddr;
    uint32_t depc;
};

typedef enum {
    REASON_DEFAULT_RST      = 0,
    REASON_WDT_RST          = 1,
    REASON_EXCEPTION_RST    = 2,
    REASON_SOFT_WDT_RST     = 3,
    REASON_SOFT_RESTART     = 4,
    REASON_DEEP_SLEEP_AWAKE = 5,
    REASON_EXT_SYS_RST      = 6
} rst_reason;

/* Interface indices for the wifi_get_* / wifi_set_* family (non-OS SDK).
 * LEAmDNS queries the SOFTAP/STATION subnets to accept only local-host mDNS
 * queries. */
#define STATION_IF      0x00
#define SOFTAP_IF       0x01

/* Address snapshot of an interface (non-OS SDK `struct ip_info`, which
 * lwIP-v2 dropped). LEAmDNS_Control uses it with wifi_get_ip_info() and
 * ip4_addr_netcmp(). */
struct ip_info {
    ip_addr_t ip;
    ip_addr_t netmask;
    ip_addr_t gw;
};

/* Soft-AP security modes (non-OS SDK `AUTH_MODE`). TestEspApi writes a
 * softap_config and switches authmode between OPEN / WPA2_PSK. */
typedef enum {
    AUTH_OPEN = 0,
    AUTH_WEP,
    AUTH_WPA_PSK,
    AUTH_WPA2_PSK,
    AUTH_WPA_WPA2_PSK,
    AUTH_MAX
} AUTH_MODE;

/* Soft-AP configuration (non-OS SDK `struct softap_config`). TestEspApi reads
 * it back with wifi_softap_get_config(); BL602 backend wraps softAPConfig(). */
struct softap_config {
    uint8 ssid[32];
    uint8 password[64];
    uint8 ssid_len;        /* Recommend to set it according to your ssid */
    uint8 channel;         /* support 1 ~ 13 */
    AUTH_MODE authmode;    /* Don't support AUTH_WEP in softAP mode */
    uint8 ssid_hidden;     /* default 0 */
    uint8 max_connection;  /* default 4, max 4 */
    uint16 beacon_interval;/* support 100 ~ 60000 ms, default 100 */
};

/* MAC formatting macros used by TestEspApi. */
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif
#ifndef MACSTR
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

/* wifi_set_opmode() bit values (non-OS SDK). interactive.ino passes these
 * directly. */
#define NULL_MODE       0x00
#define STATION_MODE    0x01
#define SOFTAP_MODE     0x02
#define STATIONAP_MODE  0x03

/* CPU frequencies for system_update_cpu_freq() (non-OS SDK). TestEspApi
 * references SYS_CPU_160MHZ. BL602 is fixed at 192 MHz; both values are
 * accepted and ignored by the backend. */
#define SYS_CPU_80MHZ   80
#define SYS_CPU_160MHZ  160

/* Sleep modes for wifi_set_sleep_type() / wifi_fpm_*. */
typedef enum {
    NONE_SLEEP_T    = 0,
    LIGHT_SLEEP_T,
    MODEM_SLEEP_T
} sleep_type_t;

/* IPv4 address blob (non-OS SDK layout, matches lwIP ip4_addr_t). */
struct ipv4_addr {
    uint32_t addr;
};

/* ---- non-OS-SDK WiFi event (TestEspApi installs a handler) ---- */
enum {
    EVENT_STAMODE_CONNECTED = 0,
    EVENT_STAMODE_DISCONNECTED,
    EVENT_STAMODE_AUTHMODE_CHANGE,
    EVENT_STAMODE_GOT_IP,
    EVENT_STAMODE_DHCP_TIMEOUT,
    EVENT_SOFTAPMODE_STACONNECTED,
    EVENT_SOFTAPMODE_STADISCONNECTED,
    EVENT_SOFTAPMODE_PROBEREQRECVED,
    EVENT_OPMODE_CHANGED,
    EVENT_SOFTAPMODE_DISTRIBUTE_STA_IP,
    EVENT_MAX
};

typedef struct {
    uint8 ssid[32];
    uint8 ssid_len;
    uint8 bssid[6];
    uint8 channel;
} Event_StaMode_Connected_t;

typedef struct {
    uint8 ssid[32];
    uint8 ssid_len;
    uint8 bssid[6];
    uint8 reason;
} Event_StaMode_Disconnected_t;

typedef struct {
    uint8 old_mode;
    uint8 new_mode;
} Event_StaMode_AuthMode_Change_t;

typedef struct {
    struct ipv4_addr ip;
    struct ipv4_addr mask;
    struct ipv4_addr gw;
} Event_StaMode_Got_IP_t;

typedef struct {
    uint8 mac[6];
    uint8 aid;
} Event_SoftAPMode_StaConnected_t;

typedef struct {
    uint8 mac[6];
    struct ipv4_addr ip;
    uint8 aid;
} Event_SoftAPMode_Distribute_Sta_IP_t;

typedef struct {
    uint8 mac[6];
    uint8 aid;
} Event_SoftAPMode_StaDisconnected_t;

typedef struct {
    int rssi;
    uint8 mac[6];
} Event_SoftAPMode_ProbeReqRecved_t;

typedef struct {
    uint8 old_opmode;
    uint8 new_opmode;
} Event_OpMode_Change_t;

typedef union {
    Event_StaMode_Connected_t           connected;
    Event_StaMode_Disconnected_t        disconnected;
    Event_StaMode_AuthMode_Change_t     auth_change;
    Event_StaMode_Got_IP_t              got_ip;
    Event_SoftAPMode_StaConnected_t     sta_connected;
    Event_SoftAPMode_StaDisconnected_t  sta_disconnected;
    Event_SoftAPMode_ProbeReqRecved_t   ap_probereqrecved;
    Event_SoftAPMode_Distribute_Sta_IP_t distribute_sta_ip;
    Event_OpMode_Change_t               opmode_changed;
} Event_Info_u;

typedef struct _esp_event {
    uint32 event;
    Event_Info_u event_info;
} System_Event_t;

typedef void (* wifi_event_handler_cb_t)(System_Event_t *event);

/* Flash-size map enum for system_get_flash_size_map(). */
enum flash_size_map {
    FLASH_SIZE_4M_MAP_256_256 = 0,
    FLASH_SIZE_2M,
    FLASH_SIZE_8M_MAP_512_512,
    FLASH_SIZE_16M_MAP_512_512,
    FLASH_SIZE_32M_MAP_512_512,
    FLASH_SIZE_16M_MAP_1024_1024,
    FLASH_SIZE_32M_MAP_1024_1024,
    FLASH_SIZE_32M_MAP_2048_2048,
    FLASH_SIZE_64M_MAP_1024_1024,
    FLASH_SIZE_128M_MAP_1024_1024
};

/* resetInfo global — defined in Esp.cpp (BL602 power-on reset). */
extern struct rst_info resetInfo;

#ifdef __cplusplus
extern "C" {
#endif

/* Non-OS-SDK IP query. BL602 backend reads the STA/AP netif directly
 * (implemented in LwipIntf.cpp). */
bool wifi_get_ip_info(uint8_t if_index, struct ip_info *info);

/* ---- non-OS-SDK system introspection (TestEspApi) ----
 * BL602 backends live in esp_compat.c; where no hardware feature exists the
 * function reports a sane default rather than failing to link. */
struct rst_info* system_get_rst_info(void);
uint32 system_get_time(void);
void system_print_meminfo(void);
uint32 system_get_free_heap_size(void);
void system_set_os_print(uint8 onoff);
uint8 system_get_os_print(void);
uint32 system_get_chip_id(void);
const char *system_get_sdk_version(void);
uint8 system_get_boot_version(void);
uint32 system_get_userbin_addr(void);
uint8 system_get_boot_mode(void);
bool system_update_cpu_freq(uint8 freq);
uint8 system_get_cpu_freq(void);
enum flash_size_map system_get_flash_size_map(void);

/* ---- non-OS-SDK wifi control (TestEspApi, interactive) ----
 * Backends live in the ESP8266WiFi library, where the radio state lives. */
uint8 wifi_get_opmode(void);
uint8 wifi_get_opmode_default(void);
bool wifi_set_opmode(uint8 opmode);
/* Non-OS-SDK STA link control (interactive 'c'/'C'). Backend in
 * ESP8266WiFiGeneric.cpp wraps WiFi.begin()/WiFi.disconnect(). */
bool wifi_station_connect(void);
bool wifi_station_disconnect(void);
uint8 wifi_get_broadcast_if(void);
uint8 wifi_get_channel(void);
bool wifi_set_channel(uint8 ch);

/* ESP8266 non-OS-SDK country config. WiFiMesh's EspnowDatabase reads the
 * country to validate its ESP-NOW channel; BL602's regulator has no country
 * concept, so the backend reports the ESP8266 default {cc="CN", 1..13} — the
 * assert `schan <= ch <= schan+nchan-1` then passes for any 2.4 GHz channel. */
typedef enum {
    WIFI_COUNTRY_POLICY_AUTO,
    WIFI_COUNTRY_POLICY_MANUAL,
} WIFI_COUNTRY_POLICY;

typedef struct {
    char     cc[3];
    uint8_t  schan;
    uint8_t  nchan;
    uint8_t  policy;
} wifi_country_t;

bool wifi_get_country(wifi_country_t *country);
bool wifi_set_country(wifi_country_t *country);
uint8 wifi_get_phy_mode(void);
bool wifi_set_phy_mode(uint8 mode);
bool wifi_set_sleep_type(sleep_type_t type);
void wifi_set_event_handler_cb(wifi_event_handler_cb_t cb);
void wifi_fpm_open(void);
void wifi_fpm_close(void);
void wifi_fpm_do_wakeup(void);
sint8 wifi_fpm_do_sleep(uint32 sleep_time_in_us);
/* Extra FPM entry points used by LowPowerDemo; BL602 has no fast-power-mode
 * state machine, so these are no-ops (implemented in ESP8266WiFiGeneric.cpp). */
void wifi_fpm_set_sleep_type(sleep_type_t type);
typedef void (*fpm_wakeup_cb_t)(void);
void wifi_fpm_set_wakeup_cb(fpm_wakeup_cb_t cb);

/* ---- legacy GPIO wakeup + pin macros (LowPowerDemo) ----
 * BL602 deep sleep / GPIO wakeup is not exposed via this SDK, so these are
 * compile-compatible no-ops (declared here, implemented in ESP8266WiFiGeneric.cpp). */
#define GPIO_ID_PIN(pin)        (pin)
#define GPIO_PIN_ADDR(pin)      (pin)
typedef enum {
    GPIO_PIN_INTR_DISABLE  = 0,
    GPIO_PIN_INTR_POSEDGE  = 1,
    GPIO_PIN_INTR_NEGEDGE  = 2,
    GPIO_PIN_INTR_ANYEDGE  = 3,
    GPIO_PIN_INTR_LOLEVEL  = 4,
    GPIO_PIN_INTR_HILEVEL  = 5
} GPIO_INT_TYPE;
void gpio_pin_wakeup_enable(uint32_t i, GPIO_INT_TYPE intr_state);
void gpio_pin_wakeup_disable(uint32_t i);

/* ---- legacy OS timer + user RTC RAM (LowPowerDemo) ----
 * os_timer_t is opaque: sketches only null `timer_list` out. RTC_USER_MEM maps
 * to a plain buffer (Esp.cpp) — BL602 exposes no retained-RAM address. */
typedef struct _os_timer_t {
    struct _os_timer_t *next;
    void *cb;
    uint32_t timer_arg;
    uint32_t expire;
    uint32_t interval;
} os_timer_t;
extern os_timer_t *timer_list;
#define RTC_USER_MEM_SIZE 512
extern uint8_t s_rtc_user_mem[RTC_USER_MEM_SIZE];
#define RTC_USER_MEM ((void *)s_rtc_user_mem)

/* Non-OS-SDK soft-AP config / station-count query. Backend is the WB2 WiFi
 * library (ESP8266WiFiAP.cpp), where these wrap softAPConfig()/softAP(). */
bool wifi_softap_get_config(struct softap_config *config);
bool wifi_softap_set_config(struct softap_config *config);
uint8 wifi_softap_get_station_num(void);

/* ---- non-OS-SDK connected-station list (StaticLease example) ----
 * wifi_softap_get_station_info() returns the head of a singly-linked list of
 * connected stations (STAILQ of struct station_info), NULL when none. BL602's
 * wifi_mgmr owns the real station table; the WB2 backend returns NULL, so a
 * sketch's walk simply sees no stations and frees nothing — compile-compatible,
 * harmless at runtime. */
struct station_config {
    uint8_t ssid[32];
    uint8_t password[64];
    uint8_t bssid_set;
    uint8_t bssid[6];
    uint8_t ssid_len;
    uint8_t password_len;
};

struct station_info {
    STAILQ_ENTRY(station_info) next;
    uint8_t bssid[6];
    ip_addr_t ip;
};

struct station_info* wifi_softap_get_station_info(void);
void wifi_softap_free_station_list(void);

/* Non-OS-SDK STA hostname. Backend keeps a hostname string (ESP8266WiFiGeneric.cpp). */
const char* wifi_station_get_hostname(void);

/* lwIP promiscuous packet-capture hook (Netdump). Declared here as on the
 * ESP8266 SDK (its lwip "gluedebug.h"). BL602's network stack has no PHY
 * capture path, so the pointer stays NULL unless a library assigns it, and
 * nothing in the stack ever invokes it — sketches compile and run, the
 * capture simply sees no packets. Backing symbol in esp_compat.c. */
extern void (*phy_capture)(int netif_idx, const char* data, size_t len, int out, int success);

#ifdef __cplusplus
}
#endif

#endif /* USER_INTERFACE_H */
