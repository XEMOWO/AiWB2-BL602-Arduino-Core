/*
 * Schedule.cpp — scheduled functions (ESP8266-compatible).
 *
 * Backed by fixed-size tables guarded with ets_intr_lock()/unlock() so the
 * one-shot queue and the recurrent table are safe to touch from an ISR or a
 * library callback while loop() runs them.
 *
 * Timing for recurrent functions uses micros() (the free-running RISC-V cycle
 * counter scaled to µs), so periods are millisecond-usable.
 */
#include "Schedule.h"

#include <string.h>

#include <Arduino.h>

typedef std::function<void(void)> sched_fn_t;
typedef std::function<bool(void)> sched_bool_fn_t;

/* ---- one-shot queue (FIFO) ---- */

static sched_fn_t s_one_shots[SCHEDULED_FN_MAX_COUNT];
static volatile uint16_t s_q_head = 0; /* next to run */
static volatile uint16_t s_q_tail = 0; /* next free slot */

/* ---- recurrent table ---- */

struct recurrent_t {
    sched_bool_fn_t fn;
    uint32_t repeat_us;
    uint32_t next_us;    /* next fire time in micros() domain */
    uint32_t has_alarm;
    sched_bool_fn_t alarm;
};

static recurrent_t s_recurrent[SCHEDULED_FN_MAX_COUNT];

/* ---- locking (delegates to the core's IRQ save/restore) ---- */
extern "C" void ets_intr_lock(void);
extern "C" void ets_intr_unlock(void);

/* ---- C hooks so wiring.c (a C file) can service the schedulers ----
 * yield()/delay() call these; the loop task calls the C++ versions directly.
 * A simple reentrancy guard keeps a scheduled function that itself calls
 * yield()/delay() from recursing. */
static volatile int s_running = 0;

#include "Ticker.h" /* service() runs alongside the schedulers */

extern "C" int wb2_sched_pending(void)
{
    uint16_t i;
    if (s_q_head != s_q_tail) {
        return 1;
    }
    for (i = 0; i < SCHEDULED_FN_MAX_COUNT; i++) {
        if (s_recurrent[i].fn) {
            return 1;
        }
    }
    return 0;
}

extern "C" void wb2_run_scheduled(void)
{
    if (s_running) {
        return;
    }
    s_running = 1;
    Ticker::service();
    run_scheduled_functions();
    run_scheduled_recurrent_functions();
    s_running = 0;
}

extern "C" void wb2_run_recurrent(void)
{
    if (s_running) {
        return;
    }
    s_running = 1;
    Ticker::service();
    run_scheduled_recurrent_functions();
    s_running = 0;
}

bool schedule_function(const std::function<void(void)> &fn)
{
    uint16_t tail;
    bool ok = false;

    if (!fn) {
        return false;
    }
    ets_intr_lock();
    tail = s_q_tail;
    if ((tail + 1) % SCHEDULED_FN_MAX_COUNT != s_q_head) {
        s_one_shots[tail] = fn;
        s_q_tail = (tail + 1) % SCHEDULED_FN_MAX_COUNT;
        ok = true;
    }
    ets_intr_unlock();
    return ok;
}

void run_scheduled_functions(void)
{
    for (;;) {
        uint16_t head;
        sched_fn_t fn;

        ets_intr_lock();
        head = s_q_head;
        if (head == s_q_tail) {
            ets_intr_unlock();
            return;
        }
        fn = s_one_shots[head];
        s_q_head = (head + 1) % SCHEDULED_FN_MAX_COUNT;
        ets_intr_unlock();

        fn();
    }
}

bool schedule_recurrent_function_us(const std::function<bool(void)> &fn,
                                    uint32_t repeat_us,
                                    const std::function<bool(void)> &alarm)
{
    uint16_t i;
    uint32_t now;

    if (!fn || repeat_us == 0) {
        return false;
    }
    now = micros();

    ets_intr_lock();
    for (i = 0; i < SCHEDULED_FN_MAX_COUNT; i++) {
        if (s_recurrent[i].fn) {
            continue;
        }
        s_recurrent[i].fn = fn;
        s_recurrent[i].repeat_us = repeat_us;
        s_recurrent[i].next_us = now + repeat_us;
        s_recurrent[i].has_alarm = (alarm ? 1 : 0);
        s_recurrent[i].alarm = alarm;
        ets_intr_unlock();
        return true;
    }
    ets_intr_unlock();
    return false;
}

static uint32_t s_recurrent_min_period(void)
{
    uint16_t i;
    uint32_t min = 0;

    ets_intr_lock();
    for (i = 0; i < SCHEDULED_FN_MAX_COUNT; i++) {
        if (s_recurrent[i].fn && (min == 0 || s_recurrent[i].repeat_us < min)) {
            min = s_recurrent[i].repeat_us;
        }
    }
    ets_intr_unlock();
    return min;
}

uint32_t compute_scheduled_recurrent_grain(void)
{
    return s_recurrent_min_period();
}

void run_scheduled_recurrent_functions(void)
{
    uint16_t i;
    uint32_t now;

    /* Cheap re-check: if nothing is registered, bail without taking locks. */
    {
        uint16_t any = 0;
        for (i = 0; i < SCHEDULED_FN_MAX_COUNT; i++) {
            if (s_recurrent[i].fn) {
                any = 1;
                break;
            }
        }
        if (!any) {
            return;
        }
    }

    now = micros();
    for (i = 0; i < SCHEDULED_FN_MAX_COUNT; i++) {
        bool fire = false;

        ets_intr_lock();
        if (!s_recurrent[i].fn) {
            ets_intr_unlock();
            continue;
        }
        if (s_recurrent[i].has_alarm && s_recurrent[i].alarm()) {
            fire = true; /* alarm fired early */
        } else if ((int32_t)(now - s_recurrent[i].next_us) >= 0) {
            fire = true; /* period elapsed (wrap-safe compare) */
        }
        ets_intr_unlock();

        if (!fire) {
            continue;
        }

        /* Run outside the lock so the callback may re-schedule or unregister
         * itself. A false return removes the entry. */
        bool keep = s_recurrent[i].fn();

        ets_intr_lock();
        if (!keep) {
            s_recurrent[i].fn = sched_bool_fn_t();
            s_recurrent[i].alarm = sched_bool_fn_t();
            ets_intr_unlock();
            continue;
        }
        if (s_recurrent[i].fn) { /* still present (may have been detached) */
            uint32_t next = now + s_recurrent[i].repeat_us;
            /* avoid firing again immediately when the loop is slow */
            if ((int32_t)(next - now) < 0) {
                next = now;
            }
            s_recurrent[i].next_us = next;
        }
        ets_intr_unlock();
    }
}
