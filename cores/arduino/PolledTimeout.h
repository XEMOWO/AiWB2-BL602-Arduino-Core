/*
 * PolledTimeout.h — polled timeout template (ESP8266-compatible).
 *
 * Ported from cores/esp8266/PolledTimeout.h (esp8266/Arduino repo,
 * Copyright (c) 2018 Daniel Salazar, LGPL 2.1+). Provides the familiar
 * oneShotMs / periodicMs / oneShot / periodic / Fast variants. TimeSourceCycles
 * uses the RISC-V cycle CSR via esp_get_cycle_count().
 *
 * Adaptations for BL602:
 *   - IRAM_ATTR expands empty (no-op; Arduino.h defines it).
 *   - TimeSourceCycles::time() uses our cycle CSR; ticksPerSecond = F_CPU.
 *
 * This header MUST NOT include <Arduino.h>. Arduino.h -> HardwareSerial.h ->
 * Stream.h -> (this header) -> namespace esp8266, and Stream.h's send*() API
 * aliases esp8266::polledTimeout::oneShotFastMs. If PolledTimeout.h pulled in
 * Arduino.h, a TU that includes PolledTimeout.h FIRST (e.g. ESP8266WiFiMulti.cpp)
 * would make Arduino.h's mid-parse inclusion of Stream.h run before the
 * `namespace esp8266` block below, failing with "esp8266 does not name a type".
 * The ESP8266 core keeps this header standalone by declaring millis()/delay()
 * in core_esp8266_features.h; we declare them here instead.
 */
#ifndef __POLLEDTIMING_H__
#define __POLLEDTIMING_H__

#include <limits>      /* std::numeric_limits */
#include <type_traits> /* std::is_unsigned */

/* Same linkage/type as the Arduino.h extern "C" declarations — declared HERE,
 * before coredecls.h, because coredecls.h's esp_delay(T&&,...) template body
 * calls millis() (non-dependent name, resolved at template-parse time). */
extern "C" {
unsigned long millis(void);
void delay(unsigned long ms);
}

#include <coredecls.h> /* esp_yield() / esp_get_cycle_count() + F_CPU fallback */

namespace esp8266 {

namespace polledTimeout {

namespace YieldPolicy {

struct DoNothing
{
    static void execute() {}
};

struct YieldOrSkip
{
    static void execute() { esp_yield(); }
};

template <unsigned long delayMs>
struct YieldAndDelayMs
{
    static void execute() { delay(delayMs); }
};

} /* YieldPolicy */

namespace TimePolicy {

struct TimeSourceMillis
{
    using timeType = decltype(millis());
    static timeType time() { return millis(); }
    static constexpr timeType ticksPerSecond    = 1000;
    static constexpr timeType ticksPerSecondMax = 1000;
};

struct TimeSourceCycles
{
    using timeType = decltype(esp_get_cycle_count());
    static timeType time() { return esp_get_cycle_count(); }
    static constexpr timeType ticksPerSecond    = F_CPU;
    static constexpr timeType ticksPerSecondMax = 192000000UL;
};

template <typename TimeSourceType, unsigned long long second_th>
struct TimeUnit
{
    using timeType = typename TimeSourceType::timeType;

    static constexpr timeType computeRangeCompensation()
    {
        return ({
            constexpr double number_of_secondTh_in_one_tick = (1.0 * second_th) / ticksPerSecond;
            constexpr double fractional = number_of_secondTh_in_one_tick - (long)number_of_secondTh_in_one_tick;
            fractional == 0
                ? 1 /* no need for compensation */
                : (number_of_secondTh_in_one_tick / fractional) + 0.5; /* multiplier allowing exact division */
        });
    }

    static constexpr timeType ticksPerSecond         = TimeSourceType::ticksPerSecond;
    static constexpr timeType ticksPerSecondMax      = TimeSourceType::ticksPerSecondMax;
    static constexpr timeType rangeCompensate        = computeRangeCompensation();
    /* Promote the multiply to 64-bit: on WB2, F_CPU = 192 MHz and
     * ticksPerSecondMax * rangeCompensate (e.g. 192e6 * 25 = 4.8e9) overflows
     * 32-bit uint, wrapping the result to 0 and making timeMax() divide by
     * zero at compile time. ESP8266 (160 MHz, 160e6*25 = 4e9) fits by luck. */
    static constexpr timeType user2UnitMultiplierMax = (ticksPerSecondMax * (unsigned long long)rangeCompensate) / second_th;
    static constexpr timeType user2UnitMultiplier    = (ticksPerSecond    * (unsigned long long)rangeCompensate) / second_th;
    static constexpr timeType user2UnitDivider       = rangeCompensate;
    static constexpr timeType timeMax                = (std::numeric_limits<timeType>::max() - 1) / user2UnitMultiplierMax;

    static timeType toTimeTypeUnit(const timeType userUnit) { return (userUnit * user2UnitMultiplier) / user2UnitDivider; }
    static timeType toUserUnit(const timeType internalUnit) { return (internalUnit * user2UnitDivider) / user2UnitMultiplier; }
    static timeType time() { return TimeSourceType::time(); }
};

using TimeMillis     = TimeUnit<TimeSourceMillis,       1000>;
using TimeFastMillis = TimeUnit<TimeSourceCycles,       1000>;
using TimeFastMicros = TimeUnit<TimeSourceCycles,    1000000>;
using TimeFastNanos  = TimeUnit<TimeSourceCycles, 1000000000>;

} /* TimePolicy */

template <bool PeriodicT, typename YieldPolicyT = YieldPolicy::DoNothing, typename TimePolicyT = TimePolicy::TimeMillis>
class timeoutTemplate
{
public:
    using timeType = typename TimePolicyT::timeType;
    static_assert(std::is_unsigned<timeType>::value == true, "timeType must be unsigned");

    static constexpr timeType alwaysExpired   = 0;
    static constexpr timeType neverExpires    = std::numeric_limits<timeType>::max();
    static constexpr timeType rangeCompensate = TimePolicyT::rangeCompensate;

    timeoutTemplate(const timeType userTimeout)
    {
        reset(userTimeout);
    }

    bool expired()
    {
        YieldPolicyT::execute(); /* DoNothing: optimized away */
        if (PeriodicT) {
            return expiredRetrigger();
        }
        return expiredOneShot();
    }

    operator bool()
    {
        return expired();
    }

    bool canExpire() const
    {
        return !_neverExpires;
    }

    bool canWait() const
    {
        return _timeout != alwaysExpired;
    }

    void reset(const timeType newUserTimeout)
    {
        reset();
        _timeout = TimePolicyT::toTimeTypeUnit(newUserTimeout);
        _neverExpires = (newUserTimeout < 0) || (newUserTimeout > timeMax());
    }

    void reset()
    {
        _start = TimePolicyT::time();
    }

    void resetAndSetExpired(const timeType newUserTimeout)
    {
        reset(newUserTimeout);
        _start -= _timeout;
    }

    void resetAndSetExpired()
    {
        reset();
        _start -= _timeout;
    }

    void resetToNeverExpires()
    {
        _timeout = alwaysExpired + 1;
        _neverExpires = true;
    }

    timeType getTimeout() const
    {
        return TimePolicyT::toUserUnit(_timeout);
    }

    static constexpr timeType timeMax()
    {
        return TimePolicyT::timeMax;
    }

private:
    bool checkExpired(const timeType internalUnit) const
    {
        return (!_neverExpires) && ((internalUnit - _start) >= _timeout);
    }

protected:
    bool expiredRetrigger()
    {
        if (!canWait()) {
            return true;
        }
        timeType current = TimePolicyT::time();
        if (checkExpired(current)) {
            unsigned long n = (current - _start) / _timeout;
            _start += n * _timeout;
            return true;
        }
        return false;
    }

    bool expiredOneShot() const
    {
        return !canWait() || checkExpired(TimePolicyT::time());
    }

    timeType _timeout;
    timeType _start;
    bool _neverExpires;
};

/* legacy type names (unit is milliseconds) */
using oneShot   = polledTimeout::timeoutTemplate<false>;
using periodic  = polledTimeout::timeoutTemplate<true>;

/* standard versions (based on millis()); timeMax() is ~49.7 days */
using oneShotMs   = polledTimeout::timeoutTemplate<false>;
using periodicMs  = polledTimeout::timeoutTemplate<true>;

/* fast versions based on the cycle counter */
using oneShotFastMs  = polledTimeout::timeoutTemplate<false, YieldPolicy::DoNothing, TimePolicy::TimeFastMillis>;
using periodicFastMs = polledTimeout::timeoutTemplate<true,  YieldPolicy::DoNothing, TimePolicy::TimeFastMillis>;
using oneShotFastUs  = polledTimeout::timeoutTemplate<false, YieldPolicy::DoNothing, TimePolicy::TimeFastMicros>;
using periodicFastUs = polledTimeout::timeoutTemplate<true,  YieldPolicy::DoNothing, TimePolicy::TimeFastMicros>;
using oneShotFastNs  = polledTimeout::timeoutTemplate<false, YieldPolicy::DoNothing, TimePolicy::TimeFastNanos>;
using periodicFastNs = polledTimeout::timeoutTemplate<true,  YieldPolicy::DoNothing, TimePolicy::TimeFastNanos>;

} /* polledTimeout */

} /* esp8266 */

#endif /* __POLLEDTIMING_H__ */
