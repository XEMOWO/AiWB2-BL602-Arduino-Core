/*
 * coredecls.h — ESP8266-compatible core free functions for BL602.
 *
 * The ESP8266 core declares these in coredecls.h / core_esp8266_features.h;
 * third-party libraries call them directly. On RISC-V the cycle counter is
 * the `cycle` CSR (increments at 192 MHz), equivalent to Xtensa ccount.
 */
#ifndef COREDECLS_H
#define COREDECLS_H

#include <stdint.h>

/* F_CPU comes from the build defines (or Arduino.h's fallback). */
#ifndef F_CPU
#define F_CPU 192000000L
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Cooperative yield (ESP8266: yield() and esp_yield() are the same). */
void esp_yield(void);

/* Blocking sleep (BL602 maps it to wiring's delay()). */
void esp_delay(unsigned long ms);

/* Set the system time base to an absolute Unix epoch in microseconds
 * (ESP8266 name; BL602 maps it to bl_sys_time_update). */
void tune_timeshift64 (uint64_t now_us);

/* legacy nonos-sdk API: set TZ (seconds east of UTC) for the current NTP
 * servers. */
bool sntp_set_timezone_in_seconds(int32_t timezone);

#ifdef __cplusplus
} /* extern "C" */

#include <functional>
#include <utility>

using BoolCB = std::function<void(bool)>;
using TrivialCB = std::function<void()>;

/* ESP8266 continuation-context pointer (g_pcont). Sketches that inspect the
 * nonos-sdk task/stack layout read it; on BL602 there is no such context, so
 * this is an inert pointer kept for source compatibility (HwdtStackDump). */
extern "C" void *g_pcont;

/* ESP8266 esp_delay() family. The (ms, blocked, intvl_ms) overload delays in
 * intvl_ms chunks, calling blocked() after each chunk; it returns once the
 * timeout elapses or blocked() reports completion. Used by WiFiMulti and other
 * third-party network libraries. */
bool esp_try_delay(const uint32_t start_ms, const uint32_t timeout_ms, const uint32_t intvl_ms);

template <typename T>
inline void esp_delay(const uint32_t timeout_ms, T&& blocked, const uint32_t intvl_ms) {
    const auto start_ms = millis();
    while (!esp_try_delay(start_ms, timeout_ms, intvl_ms) && blocked()) {
    }
}

template <typename T>
inline void esp_delay(const uint32_t timeout_ms, T&& blocked) {
    esp_delay(timeout_ms, std::forward<T>(blocked), timeout_ms);
}

/* Called whenever the wall-clock time is changed (NTP sync or manual
 * settimeofday). `bool` argument is true when the source was NTP. */
void settimeofday_cb (BoolCB&& cb);
void settimeofday_cb (const BoolCB& cb);
void settimeofday_cb (const TrivialCB& cb);
#endif

/* Free-running CPU cycle counter. Same 32-bit wrap semantics as ccount. */
inline uint32_t esp_get_cycle_count() __attribute__((always_inline));
inline uint32_t esp_get_cycle_count()
{
    uint32_t ccount;
    __asm__ volatile("csrr %0, cycle" : "=r"(ccount));
    return ccount;
}

/* CRC-32 (IEEE 802.3, reflected, poly 0x04c11db7) — the ESP8266 coredecls.h
 * crc32(). Used by OTASdkCheck to fingerprint the SDK version string. */
inline uint32_t crc32(const void* data, size_t length, uint32_t crc = 0xffffffff)
{
    const uint8_t* ldata = (const uint8_t*)data;
    while (length--) {
        uint8_t c = *ldata++;
        for (uint32_t i = 0x80; i > 0; i >>= 1) {
            bool bit = crc & 0x80000000;
            if (c & i)
                bit = !bit;
            crc <<= 1;
            if (bit)
                crc ^= 0x04c11db7;
        }
    }
    return crc;
}

/* Current program counter (ESP8266 core_esp8266_features.h). Xtensa reads it
 * with `movi %0, .`; RISC-V has no PC read, so auipc(0) yields its own address.
 * Used by the exception-decode helpers in examples like HelloServer. */
inline uint32_t esp_get_program_counter() __attribute__((always_inline));
inline uint32_t esp_get_program_counter()
{
    uint32_t pc;
    __asm__ __volatile__("1: auipc %0, 0" : "=r"(pc));
    return pc;
}

/* CPU frequency in MHz (192 on BL602). */
inline int esp_get_cpu_freq_mhz()
{
    return (int)(F_CPU / 1000000UL);
}

#endif /* COREDECLS_H */
