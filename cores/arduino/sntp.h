/*
 * sntp.h — top-level SNTP header for ESP8266 sketches.
 *
 * On ESP8266, `#include <sntp.h>` pulls the non-OS-SDK SNTP client (a wrapper
 * over lwIP's apps/sntp). The BL602 SDK ships the same lwIP sntp as its own
 * `-lsntp` library and declares it in <lwip/apps/sntp.h> — which is exactly
 * what the ESP8266 header does. This shim forwards to it, then adds the
 * ESP8266 time helpers that our time.cpp implements (sntp_get_current_timestamp,
 * sntp_get_real_time, sntp_get/set_timezone).
 *
 * The SDK's sntp links against the prebuilt libsntp.a, so these resolve at
 * link time; sntp_init() polls and calls settimeofday() when the clock syncs.
 */
#ifndef __SNTP_H__
#define __SNTP_H__

#include <stdint.h>
#include <time.h>

/* SDK lwIP sntp (declarations for sntp_init/sntp_stop/sntp_setservername/
 * sntp_getservername/sntp_getserver/sntp_getreachability/SNTP_MAX_SERVERS/...). */
#include <lwip/apps/sntp.h>

#include <c_types.h> /* sint8 for the ESP8266 timezone helpers */

#ifdef __cplusplus
extern "C" {
#endif

/* Implemented in time.cpp (backport of the Espressif non-OS-SDK helpers). */
uint32_t sntp_get_current_timestamp(void);
char* sntp_get_real_time(time_t t);
sint8 sntp_get_timezone(void);
bool sntp_set_timezone(sint8 timezone);

#ifdef __cplusplus
}
#endif

#endif /* __SNTP_H__ */
