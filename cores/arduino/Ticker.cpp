/*
 * Ticker.cpp — periodic callback timer (ESP8266-compatible), loop-driven.
 *
 * See Ticker.h for the design notes. The armed-ticker list is guarded with
 * ets_intr_lock() so a sketch may attach/detach while service() iterates.
 */
#include "Ticker.h"

#include <Arduino.h>
#include <Schedule.h>

extern "C" void ets_intr_lock(void);
extern "C" void ets_intr_unlock(void);

static Ticker *s_head = nullptr;

Ticker::~Ticker()
{
    detach();
}

Ticker::Ticker(const Ticker &other)
{
    (void)other; /* copies are inert, like the reference */
}

Ticker &Ticker::operator=(const Ticker &other)
{
    (void)other;
    detach();
    return *this;
}

Ticker::Ticker(Ticker &&other) noexcept
{
    other.detach(); /* like the reference: moving abandons the source */
}

Ticker &Ticker::operator=(Ticker &&other) noexcept
{
    other.detach();
    detach();
    return *this;
}

void Ticker::_attach(uint32_t milliseconds, bool repeat, bool scheduled, callback_function_t cb)
{
    if (milliseconds == 0) {
        milliseconds = 1;
    }
    detach();
    _period_ms = milliseconds;
    _repeat = repeat;
    _scheduled = scheduled;
    _callback = std::move(cb);
    _next_ms = millis() + milliseconds;
    ++_arm_id;

    ets_intr_lock();
    _next_list = s_head;
    s_head = this;
    ets_intr_unlock();
}

void Ticker::attach_scheduled(float seconds, callback_function_t callback)
{
    _attach(secondsToMs(seconds), true, true, [callback]() { schedule_function(callback); });
}

void Ticker::attach_ms_scheduled(uint32_t milliseconds, callback_function_t callback)
{
    _attach(milliseconds, true, true, [callback]() { schedule_function(callback); });
}

void Ticker::attach_ms_scheduled_accurate(uint32_t milliseconds, callback_function_t callback)
{
    /* fires at the next yield() rather than loop(): equivalent to a recurrent
     * function registered with a 0-period on ESP8266 (runs once, then drops). */
    _attach(milliseconds, true, true,
            [callback]() { schedule_recurrent_function_us(
                [callback]() { callback(); return false; }, 1); });
}

void Ticker::attach(float seconds, callback_function_t callback)
{
    _attach(secondsToMs(seconds), true, false, std::move(callback));
}

void Ticker::attach_ms(uint32_t milliseconds, callback_function_t callback)
{
    _attach(milliseconds, true, false, std::move(callback));
}

void Ticker::once_scheduled(float seconds, callback_function_t callback)
{
    _attach(secondsToMs(seconds), false, true, [callback]() { schedule_function(callback); });
}

void Ticker::once_ms_scheduled(uint32_t milliseconds, callback_function_t callback)
{
    _attach(milliseconds, false, true, [callback]() { schedule_function(callback); });
}

void Ticker::once(float seconds, callback_function_t callback)
{
    _attach(secondsToMs(seconds), false, false, std::move(callback));
}

void Ticker::once_ms(uint32_t milliseconds, callback_function_t callback)
{
    _attach(milliseconds, false, false, std::move(callback));
}

void Ticker::detach()
{
    if (_period_ms == 0) {
        return;
    }
    ets_intr_lock();
    /* unlink from the list */
    Ticker **pp = &s_head;
    while (*pp && *pp != this) {
        pp = &(*pp)->_next_list;
    }
    if (*pp == this) {
        *pp = this->_next_list;
    }
    ets_intr_unlock();

    _period_ms = 0;
    _callback = callback_function_t();
    _next_list = nullptr;
}

bool Ticker::active() const
{
    return _period_ms != 0;
}

void Ticker::service()
{
    Ticker *t = s_head;
    if (!t) {
        return;
    }
    uint32_t now = millis();

    while (t) {
        Ticker *next = t->_next_list; /* may be unlinked by the callback */
        uint32_t arm = t->_arm_id;

        if (t->_period_ms && (int32_t)(now - t->_next_ms) >= 0) {
            bool keep = t->_repeat;

            /* run the callback (may attach/detach any ticker, incl. us) */
            if (t->_callback) {
                t->_callback();
            }

            if (t->_period_ms && t->_arm_id == arm) {
                /* still armed and not re-attached mid-callback */
                if (keep) {
                    t->_next_ms = now + t->_period_ms;
                } else {
                    t->detach(); /* one-shot fired */
                }
            }
            /* else: callback detached us, or re-attached (new _arm_id) — _attach
             * already set a fresh deadline, leave it alone. */
        }
        t = next;
    }
}
