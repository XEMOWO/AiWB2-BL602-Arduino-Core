/*
 * WB2WiFiCommon.h — shared internals for the Ai-WB2-12F ESP8266WiFi library.
 *
 * Include order matters (same rule as the old WiFi.cpp / esp_utils.c):
 * the SDK + lwIP headers must come BEFORE Arduino.h. FreeRTOS's platform.h
 * (pulled in transitively) defines a dead `GPIO_REG` macro; wiring_private.h
 * inside Arduino.h #undef's it and re-defines it to the real GLB register.
 *
 * lwIP builds with LWIP_COMPAT_SOCKETS==1, which turns the POSIX socket names
 * (socket/connect/bind/listen/accept/read/write/close/send/recv/...) into
 * function-like macros aliasing lwip_*. Those collide with the method names of
 * the Arduino Client/Server/UDP classes, so every one of them is #undef'd (see
 * lwip_compat_undef.h) and this library always calls the explicit `lwip_*`
 * functions.
 */
#ifndef WB2WIFI_COMMON_H
#define WB2WIFI_COMMON_H

/* ---- SDK headers first (GPIO_REG ordering, see above) ---- */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/tcpip.h>
#include <lwip/init.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <lwip/dns.h>
#include <lwip/def.h>     /* htons/htonl/ntohs/ntohl */
#include <hal_wifi.h>
#include <wifi_mgmr_ext.h>
#include <bl_os_adapter/bl_os_system.h>   /* bl_os_malloc/bl_os_free for the scan array */

/* ---- undef lwIP COMPAT_SOCKETS macros that clash with the class API ---- */
#include "lwip_compat_undef.h"

#include <Arduino.h>
#include <wl_definitions.h>
#include "ESP8266WiFiType.h"   /* WiFiMode_t, WiFiEvent_t, ... */

/* ---- shared backend state (defined in ESP8266WiFiGeneric.cpp) ---- */

/* Interface handles returned by wifi_mgmr_sta_enable()/ap_enable(). */
extern wifi_interface_t wb2_sta_iface;
extern wifi_interface_t wb2_ap_iface;

/* True once tcpip_init + hal_wifi_start_firmware_task + wifi_mgmr_start_background
 * have been done for this boot. */
extern bool wb2_wifi_started;

/* Radio mode last requested through WiFi.mode(). */
extern WiFiMode_t wb2_wifi_mode;

/* Idempotent backend bring-up: tcpip_init once, start the firmware task,
 * register the yloop EV_WIFI filter, start the wifi_mgmr background manager.
 * Called from WiFi.mode()/begin()/scanNetworks() so a sketch only has to touch
 * WiFi for the radio to come up. */
void wb2_wifi_start(void);

/* Current wifi_mgmr station state mapped to the ESP8266 wl_status_t vocabulary. */
wl_status_t wb2_sta_wl_status(void);

/* yloop EV_WIFI filter. Dispatches SDK events into the ESP8266 WiFiEvent_*
 * callbacks registered through ESP8266WiFiGenericClass. Lives in the common
 * TU because both the station and the generic code want it. */
void wb2_wifi_event_filter(input_event_t *event, void *priv);

#endif /* WB2WIFI_COMMON_H */
