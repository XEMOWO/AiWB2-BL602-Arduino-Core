/*
 * wiring_timer.c — one hardware timer with an ISR callback.
 *
 *   timer_begin(period_us, cb)   configure and (re)start a periodic timer.
 *   timer_set_period(period_us)  change the period of a running timer.
 *   timer_start() / timer_stop() enable / disable the interrupt.
 *
 * The callback runs in interrupt context — keep it short, no delay()/
 * Serial. There is a single hardware timer available for user code; millis()
 * uses the free-running mtime counter, so it is unaffected.
 */
#include "Arduino.h"

#include <string.h>
#include <hosal_timer.h>

static hosal_timer_dev_t s_timer;
static void (*s_cb)(void) = NULL;
static uint8_t s_inited = 0;
static uint8_t s_started = 0;

static void timer_dispatch(void *arg)
{
    (void)arg;
    if (s_cb) {
        s_cb();
    }
}

void timer_begin(uint32_t period_us, void (*cb)(void))
{
    s_cb = cb;

    /* period is programmed into the hardware at init (matchVal), so changing
     * it requires a full re-init. */
    if (s_inited) {
        hosal_timer_stop(&s_timer);
        hosal_timer_finalize(&s_timer);
        s_inited = 0;
        s_started = 0;
    }

    memset(&s_timer, 0, sizeof s_timer);
    s_timer.port = 0;
    s_timer.config.period = period_us;
    s_timer.config.reload_mode = TIMER_RELOAD_PERIODIC;
    s_timer.config.cb = timer_dispatch;
    s_timer.config.arg = NULL;

    if (hosal_timer_init(&s_timer) != 0) {
        return;
    }
    s_inited = 1;

    if (hosal_timer_start(&s_timer) != 0) {
        return;
    }
    s_started = 1;
}

void timer_set_period(uint32_t period_us)
{
    if (!s_inited) {
        return;
    }
    uint8_t was_started = s_started;
    hosal_timer_stop(&s_timer);
    hosal_timer_finalize(&s_timer);
    s_inited = 0;
    s_started = 0;

    memset(&s_timer, 0, sizeof s_timer);
    s_timer.port = 0;
    s_timer.config.period = period_us;
    s_timer.config.reload_mode = TIMER_RELOAD_PERIODIC;
    s_timer.config.cb = timer_dispatch;
    s_timer.config.arg = NULL;

    if (hosal_timer_init(&s_timer) != 0) {
        return;
    }
    s_inited = 1;
    if (was_started && hosal_timer_start(&s_timer) == 0) {
        s_started = 1;
    }
}

void timer_start(void)
{
    if (!s_inited || s_started) {
        return;
    }
    if (hosal_timer_start(&s_timer) == 0) {
        s_started = 1;
    }
}

void timer_stop(void)
{
    if (s_started) {
        hosal_timer_stop(&s_timer);
        s_started = 0;
    }
}
