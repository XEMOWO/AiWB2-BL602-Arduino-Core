/*
 * Schedule.h — scheduled functions (ESP8266-compatible).
 *
 * Two mechanisms, matching the esp8266/Arduino Schedule.h API:
 *   - schedule_function(): queue a std::function to run once from the next
 *     loop() iteration (FIFO).
 *   - schedule_recurrent_function_us(): register a std::function<bool()> to
 *     run periodically; returning false unregisters it.
 *
 * run_scheduled_functions() / run_scheduled_recurrent_functions() are invoked
 * by the core's loop task (and yield()), exactly like the ESP8266 core.
 */
#ifndef ESP_SCHEDULE_H
#define ESP_SCHEDULE_H

#include <functional>
#include <stdint.h>

#define SCHEDULED_FN_MAX_COUNT 32

/* Returns the smallest recurrent period (µs) currently scheduled, or 0 when
 * none. delay() uses this to give recurrent functions a chance to run. */
uint32_t compute_scheduled_recurrent_grain(void);

/* Queue a one-shot function; runs at the next run_scheduled_functions().
 * Returns false if the queue is full. */
bool schedule_function(const std::function<void(void)> &fn);

/* Run all queued one-shot functions (called from loop()). */
void run_scheduled_functions(void);

/* Register a periodic function. fn is called about every repeat_us µs and
 * stops when it returns false. alarm, when given, lets fn fire early.
 * Returns false if the table is full. */
bool schedule_recurrent_function_us(const std::function<bool(void)> &fn,
                                    uint32_t repeat_us,
                                    const std::function<bool(void)> &alarm = std::function<bool(void)>());

/* Convenience: same as schedule_recurrent_function_us(fn, repeat_ms * 1000). */
inline bool schedule_recurrent_function(const std::function<bool(void)> &fn,
                                        uint32_t repeat_ms)
{
    return schedule_recurrent_function_us(fn, repeat_ms * 1000UL);
}

/* Test and run recurrent functions (called from loop() and yield()). */
void run_scheduled_recurrent_functions(void);

#endif /* ESP_SCHEDULE_H */
