/*
 * Ticker.h — periodic callback timer (ESP8266-compatible).
 *
 * API surface mirrors libraries/Ticker/src/Ticker.h of the esp8266/Arduino
 * repo (Copyright (c) 2014 Ivan Grokhotkov, LGPL 2.1+), adapted for BL602:
 *
 *   - No NONOS ets_timer; active tickers live on a linked list that the core
 *     services from loop()/yield()/delay(). This is deterministic and needs
 *     no OS timer task, so it is safe on every WB2 build.
 *   - `attach_*` callbacks fire in the loop/yield context directly (like the
 *     ESP8266 SYS context for short callbacks); `*_scheduled` variants defer
 *     the actual callback to the next loop() via schedule_function().
 *   - std::function storage only (C++11); the (Func, Arg) template forms wrap
 *     into a lambda. No std::variant, no std::chrono.
 */
#ifndef Ticker_h
#define Ticker_h

#include <functional>
#include <type_traits>

#include <Arduino.h>

class Ticker
{
public:
    using callback_function_t = std::function<void()>;
    using callback_with_arg_t = void (*)(void *);

    template <typename T>
    using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    template <typename T, typename Y = remove_cvref_t<T>>
    using callback_with_typed_arg_t = void (*)(Y);

    Ticker() = default;
    ~Ticker();
    Ticker(const Ticker &);
    Ticker &operator=(const Ticker &);
    Ticker(Ticker &&) noexcept;
    Ticker &operator=(Ticker &&) noexcept;

    /* callback deferred to the next loop() after it fires */
    void attach_scheduled(float seconds, callback_function_t callback);
    void attach_ms_scheduled(uint32_t milliseconds, callback_function_t callback);
    /* callback deferred to the next yield() after it fires */
    void attach_ms_scheduled_accurate(uint32_t milliseconds, callback_function_t callback);

    /* callback runs directly when the timer fires (loop/yield context) */
    void attach(float seconds, callback_function_t callback);
    void attach_ms(uint32_t milliseconds, callback_function_t callback);

    template <typename Func, typename Arg>
    void attach(float seconds, Func func, Arg arg)
    {
        callback_function_t cb = [func, arg]() { func(arg); };
        _attach(secondsToMs(seconds), true, false, cb);
    }
    template <typename Func, typename Arg>
    void attach_ms(uint32_t milliseconds, Func func, Arg arg)
    {
        callback_function_t cb = [func, arg]() { func(arg); };
        _attach(milliseconds, true, false, cb);
    }

    void once_scheduled(float seconds, callback_function_t callback);
    void once_ms_scheduled(uint32_t milliseconds, callback_function_t callback);
    void once(float seconds, callback_function_t callback);
    void once_ms(uint32_t milliseconds, callback_function_t callback);

    template <typename Func, typename Arg>
    void once(float seconds, Func func, Arg arg)
    {
        callback_function_t cb = [func, arg]() { func(arg); };
        _attach(secondsToMs(seconds), false, false, cb);
    }
    template <typename Func, typename Arg>
    void once_ms(uint32_t milliseconds, Func func, Arg arg)
    {
        callback_function_t cb = [func, arg]() { func(arg); };
        _attach(milliseconds, false, false, cb);
    }

    void detach();
    bool active() const;
    explicit operator bool() const { return active(); }

    /* Internal: service all armed tickers. Called by the core's Schedule
     * hooks (Schedule.cpp) from loop()/yield()/delay(). */
    static void service();

private:
    static uint32_t secondsToMs(float seconds) { return (uint32_t)(seconds * 1000.0f); }

    void _attach(uint32_t milliseconds, bool repeat, bool scheduled, callback_function_t cb);

    callback_function_t _callback;
    uint32_t _period_ms;
    uint32_t _next_ms;
    uint32_t _arm_id;      /* bumped on every _attach; detects re-attach mid-callback */
    bool _repeat;
    bool _scheduled;
    Ticker *_next_list;

    friend class TickerListAccess; /* (reserved) */
};

#endif /* Ticker_h */
