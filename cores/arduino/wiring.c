/*
 * wiring.c — time functions.
 *
 * BL602 has a free-running RISC-V mtime counter (10 ticks/us) that the SDK
 * boot code enables, so bl_timer_now_us() works without any init.
 */
#include "Arduino.h"

#include <FreeRTOS.h>
#include <task.h>
#include <bl_timer.h>

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
        return;
    }
    /* Yield to the scheduler for >=1ms delays. */
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void delayMicroseconds(unsigned int us)
{
    bl_timer_delay_us((uint32_t)us);
}
