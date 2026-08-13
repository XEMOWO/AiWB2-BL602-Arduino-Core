/*
 * time.cpp - ESP8266-compatible time functions for the Ai-WB2-12F (BL602)
 * Copyright (c) 2015 Peter Dobler. All rights reserved.
 *
 * Ported from the ESP8266 core (cores/esp8266/time.cpp), which is licensed
 * under the LGPL 2.1; see the ESP8266 core for the full license text.
 *
 * Changes for BL602:
 *   - time source is the SDK's bl_sys_time (RTC epoch ms, advanced by the
 *     FreeRTOS tick), not micros64()+timeshift64. time()/_gettimeofday_r/
 *     clock_gettime all read bl_sys_time_get().
 *   - `time`, `_gettimeofday_r`, `gettimeofday`, `clock_gettime` and
 *     `settimeofday` are all provided in this single translation unit so the
 *     toolchain newlib's time.o / gettimeofdayr.o / sysgettod.o are never
 *     pulled into the link (their `time`/`_gettimeofday_r`/`gettimeofday`
 *     symbols are resolved here, avoiding any multiple-definition risk).
 *   - settimeofday() syncs bl_sys_time instead of retargeting a shift.
 *   - NTP server strings are recorded into the SDK's lwip sntp module
 *     (sntp_setservername); sntp_init() is intentionally NOT called here yet
 *     (Phase D wires it up once the network stack is live). Until then the
 *     clock advances from boot-epoch, exactly like an ESP8266 before NTP.
 */

#include <Arduino.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <utility>

/* Weak: the ESP8266 core doesn't provide getLocalTime (it's an ESP32 API), so
 * sketches that use it either define it themselves (e.g. LittleFS_Timestamp) or
 * rely on this core-provided one. Weak lets a sketch's own definition win. */
__attribute__((weak)) bool getLocalTime(struct tm * info, uint32_t ms)
{
    uint32_t start = millis();
    time_t now;
    while((millis()-start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if(info->tm_year > (2016 - 1900)){
            return true;
        }
        delay(10);
    }
    return false;
}

#include <sys/time.h>
#include <sys/reent.h>
#include <errno.h>

extern "C" {
    #include <sys/_tz_structs.h>
    #include <bl_sys_time.h>
}

/* SDK lwip sntp (components/network/sntp) provides these with C linkage.
 * Declared here rather than including <sntp.h>, which is not on the SDK
 * include path. Only the server-name recording API is used before Phase D. */
extern "C" void sntp_stop(void);
extern "C" void sntp_init(void);
extern "C" void sntp_setservername(unsigned char idx, const char *server);
extern "C" const char *sntp_getservername(unsigned char idx);

#include <coredecls.h>
#include <Schedule.h>

/* set time base (all in ms); also what tune_timeshift64() maps to */
static uint64_t wb2_epoch_ms(void)
{
    uint64_t ms = 0;
    bl_sys_time_get(&ms);
    return ms;
}

static void setServer(int id, const char* name_or_ip)
{
    if (name_or_ip)
    {
        sntp_setservername((unsigned char) id, name_or_ip);
    }
}

static BoolCB _settimeofday_cb;

void settimeofday_cb (const TrivialCB& cb)
{
    _settimeofday_cb = [cb](bool sntp) { (void)sntp; cb(); };
}

void settimeofday_cb (BoolCB&& cb)
{
    _settimeofday_cb = std::move(cb);
}

void settimeofday_cb (const BoolCB& cb)
{
    _settimeofday_cb = cb;
}

extern "C" {

void tune_timeshift64 (uint64_t now_us)
{
    bl_sys_time_update(now_us / 1000);
}

int clock_gettime(clockid_t unused, struct timespec *tp)
{
    (void) unused;
    uint64_t m = wb2_epoch_ms();
    tp->tv_sec  = m / 1000;
    tp->tv_nsec = (m % 1000) * 1000000;
    return 0;
}

time_t time(time_t * t)
{
    time_t currentTime_s = (time_t)(wb2_epoch_ms() / 1000);
    if (t)
    {
        *t = currentTime_s;
    }
    return currentTime_s;
}

int _gettimeofday_r(struct _reent* unused, struct timeval *tp, void *tzp)
{
    (void) unused;
    (void) tzp;
    if (tp)
    {
        uint64_t currentTime_ms = wb2_epoch_ms();
        tp->tv_sec  = currentTime_ms / 1000;
        tp->tv_usec = (currentTime_ms % 1000) * 1000;
    }
    return 0;
}

int gettimeofday(struct timeval *tp, void *tzp)
{
    return _gettimeofday_r(_REENT, tp, tzp);
}

int settimeofday(const struct timeval* tv, const struct timezone* tz)
{
    bool from_sntp;
    if (tz == (struct timezone*)0xFeedC0de)
    {
        /* Special constant used by lwip SNTP calling settimeofday(...,
           0xfeedc0de) to flag an SNTP-originated time change without
           duplicating this function. */
        tz = nullptr;
        from_sntp = true;
    }
    else
        from_sntp = false;

    if (tz || !tv)
        return EINVAL;

    bl_sys_time_update(((uint64_t)tv->tv_sec) * 1000 + (tv->tv_usec / 1000));

    if (_settimeofday_cb)
        schedule_recurrent_function_us([from_sntp](){ _settimeofday_cb(from_sntp); return false; }, 0);

    return 0;
}

}; // extern "C"

///////////////////////////////////////////
// backport legacy nonos-sdk Espressif api

bool sntp_set_timezone_in_seconds (int32_t timezone_sec)
{
    configTime(timezone_sec, 0, sntp_getservername(0), sntp_getservername(1), sntp_getservername(2));
    return true;
}

bool sntp_set_timezone(int8_t timezone_in_hours)
{
    return sntp_set_timezone_in_seconds(3600 * ((int)timezone_in_hours));
}

sint8 sntp_get_timezone(void)
{
    /* _timezone is set by configTime() as timezone_sec + daylightOffset_sec. */
    return (sint8)(_timezone / 3600);
}

char* sntp_get_real_time(time_t t)
{
    return ctime(&t);
}

uint32_t sntp_get_current_timestamp()
{
    return time(nullptr);
}

// backport legacy nonos-sdk Espressif api
///////////////////////////////////////////

void configTime(int timezone_sec, int daylightOffset_sec, const char* server1, const char* server2, const char* server3)
{
    sntp_stop();

    // There is no way to tell when DST starts or stop with this API
    // So DST is always integrated in TZ
    // The other API should be preferred

    /*** hack for newlib internal timezone structures (no sprintf/sscanf) ***/

    static char gmt[] = "GMT";

    _timezone = timezone_sec + daylightOffset_sec;
    _daylight = 0;
    _tzname[0] = gmt;
    _tzname[1] = gmt;
    auto tz = __gettzinfo();
    tz->__tznorth = 1;
    tz->__tzyear = 0;
    for (int i = 0; i < 2; i++)
    {
        auto tzr = &tz->__tzrule[i];
        tzr->ch = 74;
        tzr->m = 0;
        tzr->n = 0;
        tzr->d = 0;
        tzr->s = 0;
        tzr->change = 0;
        tzr->offset = -_timezone;
    }

    /*** end of hack ***/

    // sntp servers
    setServer(0, server1);
    setServer(1, server2);
    setServer(2, server3);

    sntp_init();   /* start polling; a no-op when the network is not up yet */
}

void configTime(int timezone_sec, int daylightOffset_sec, String server1, String server2, String server3)
{
    static String servers[3];
    servers[0] = std::move(server1);
    servers[1] = std::move(server2);
    servers[2] = std::move(server3);

    configTime(timezone_sec, daylightOffset_sec,
        servers[0].length() ? servers[0].c_str() : nullptr,
        servers[1].length() ? servers[1].c_str() : nullptr,
        servers[2].length() ? servers[2].c_str() : nullptr);
}

void setTZ(const char* tz){

    char tzram[strlen_P(tz) + 1];
    memcpy_P(tzram, tz, sizeof(tzram));
    setenv("TZ", tzram, 1/*overwrite*/);
    tzset();
}

void configTime(const char* tz, const char* server1, const char* server2, const char* server3)
{
    sntp_stop();

    setServer(0, server1);
    setServer(1, server2);
    setServer(2, server3);
    setTZ(tz);

    sntp_init();   /* start polling; a no-op when the network is not up yet */
}

void configTime(const char* tz, String server1, String server2, String server3)
{
    static String servers[3];
    servers[0] = std::move(server1);
    servers[1] = std::move(server2);
    servers[2] = std::move(server3);

    configTime(tz,
        servers[0].length() ? servers[0].c_str() : nullptr,
        servers[1].length() ? servers[1].c_str() : nullptr,
        servers[2].length() ? servers[2].c_str() : nullptr);
}
