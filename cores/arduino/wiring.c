/*
 * wiring.c — time functions.
 *
 * BL602 has a free-running RISC-V mtime counter (10 ticks/us) that the SDK
 * boot code enables, so bl_timer_now_us() works without any init.
 */
/* SDK headers first: FreeRTOS's platform.h defines a dead GPIO_REG macro;
 * wiring_private.h later #undef's it for the real GLB register (see esp_utils.c). */
#include <FreeRTOS.h>
#include <task.h>
#include <bl_timer.h>

#include "Arduino.h"

unsigned long millis(void)
{
    return (unsigned long)(bl_timer_now_us() / 1000UL);
}

unsigned long micros(void)
{
    return (unsigned long)bl_timer_now_us();
}

void delay(unsigned long ms)
{
    if (ms == 0) {
        yield();   /* ESP8266: delay(0) gives background tasks a chance */
        return;
    }
    /* When Ticker/Schedule functions are registered, chunk the delay so the
     * periodic callbacks keep firing while the sketch blocks (ESP8266's delay
     * does the same via compute_scheduled_recurrent_grain). Otherwise a single
     * vTaskDelay is fine and cheaper. */
    if (wb2_sched_pending()) {
        unsigned long step = 10; /* ms per slice */
        while (ms) {
            unsigned long chunk = (ms > step) ? step : ms;
            vTaskDelay(pdMS_TO_TICKS(chunk));
            ms -= chunk;
            wb2_run_scheduled();
        }
    } else {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

void yield(void)
{
    /* Service recurrent schedulers (Ticker periodic), then let lower-priority
     * tasks (lwIP, wifi_mgmr, BLE) run. Matches ESP8266's yield(). */
    wb2_run_recurrent();
    taskYIELD();
}

void delayMicroseconds(unsigned int us)
{
    bl_timer_delay_us((uint32_t)us);
}
