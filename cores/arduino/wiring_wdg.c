/*
 * wiring_wdg.c — hardware watchdog + software reset.
 *
 *   watchdogEnable(ms)   arm the WDT; you must call watchdogFeed() before the
 *                        timeout or the chip resets.
 *   watchdogFeed()       pet the dog.
 *   esp_restart()        software reset via the WDT (arm 100 ms, never feed).
 */
#include "Arduino.h"

#include <string.h>
#include <hosal_wdg.h>

static hosal_wdg_dev_t s_wdg;
static uint8_t s_started = 0;

void watchdogEnable(uint32_t timeout_ms)
{
    if (s_started) {
        return;
    }
    if (timeout_ms == 0) {
        timeout_ms = 1;
    }
    memset(&s_wdg, 0, sizeof s_wdg);
    s_wdg.port = 0;
    s_wdg.config.timeout = timeout_ms;
    if (hosal_wdg_init(&s_wdg) == 0) {
        s_started = 1;
    }
}

void watchdogFeed(void)
{
    if (s_started) {
        hosal_wdg_reload(&s_wdg);
    }
}

void watchdogDisable(void)
{
    if (s_started) {
        hosal_wdg_finalize(&s_wdg);
        s_started = 0;
    }
}

void esp_restart(void)
{
    /* Arm the WDT with a short timeout and never reload it. */
    static hosal_wdg_dev_t r;
    memset(&r, 0, sizeof r);
    r.port = 0;
    r.config.timeout = 100;
    hosal_wdg_init(&r);
    for (;;) {
        /* spin; the WDT fires the reset */
    }
}
