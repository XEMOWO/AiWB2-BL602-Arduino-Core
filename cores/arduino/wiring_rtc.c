/*
 * wiring_rtc.c — internal real-time clock (Unix epoch seconds).
 *
 *   rtc_set_time(sec)   set the RTC to a Unix timestamp.
 *   rtc_get_time()      read it back (0 on error / before set).
 *
 * The BL602 RTC keeps counting across resets as long as its 32 kHz clock
 * source stays alive; with the internal RC it is still valid but less
 * accurate than with a 32.768 kHz crystal.
 */
#include "Arduino.h"

#include <string.h>
#include <hosal_rtc.h>

static hosal_rtc_dev_t s_rtc;
static uint8_t s_inited = 0;

static void rtc_ensure(void)
{
    if (s_inited) {
        return;
    }
    memset(&s_rtc, 0, sizeof s_rtc);
    s_rtc.port = 0;
    s_rtc.config.format = HOSAL_RTC_FORMAT_DEC;
    if (hosal_rtc_init(&s_rtc) == 0) {
        s_inited = 1;
    }
}

void rtc_set_time(uint32_t sec)
{
    uint64_t t = (uint64_t)sec;
    rtc_ensure();
    if (s_inited) {
        hosal_rtc_set_count(&s_rtc, &t);
    }
}

uint32_t rtc_get_time(void)
{
    uint64_t t = 0;
    rtc_ensure();
    if (s_inited && hosal_rtc_get_count(&s_rtc, &t) == 0) {
        return (uint32_t)t;
    }
    return 0;
}
