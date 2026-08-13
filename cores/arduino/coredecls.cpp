/*
 * coredecls.cpp — implementations of the coredecls.h free functions.
 *
 * On ESP8266, esp_yield() is the cooperative scheduler yield used by blocking
 * network/stream operations (WiFiClient::stop, Serial.flush, etc.). On BL602
 * it must pump the same software machinery that wiring's yield() does:
 * scheduled/recurrent callbacks (Schedule.cpp + Ticker::service) and a
 * cooperative reschedule so the FreeRTOS idle task keeps running.
 */
#include <Arduino.h>

/* Inert ESP8266 continuation context — see coredecls.h. */
extern "C" void *g_pcont = NULL;

void esp_yield(void)
{
    yield();
}

void esp_delay(unsigned long ms)
{
    delay(ms);
}

bool esp_try_delay(const uint32_t start_ms, const uint32_t timeout_ms, const uint32_t intvl_ms)
{
    if (!timeout_ms) {
        esp_yield();
        return true;   /* no timeout configured: treat as expired */
    }

    uint32_t expired = millis() - start_ms;
    if (expired >= timeout_ms) {
        return true;   /* expired */
    }

    /* sleep one chunk, but never past the timeout */
    uint32_t chunk = intvl_ms;
    uint32_t remaining = timeout_ms - expired;
    if (chunk > remaining) {
        chunk = remaining;
    }
    delay(chunk);

    return false;   /* caller must re-check blocked() */
}
