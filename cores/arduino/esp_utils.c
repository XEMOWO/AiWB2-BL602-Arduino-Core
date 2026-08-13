/*
 * esp_utils.c — ESP32-style system utility functions.
 *
 *   esp_get_free_heap_size()  → FreeRTOS heap remaining (xPortGetFreeHeapSize)
 *   getCpuFrequencyMhz()      → BL602 BCLK / CPU frequency (192 MHz)
 *   esp_get_chip_id()         → derived from the efuse MAC (no chip-ID efuse
 *                               on BL602, so the first 4 MAC bytes are used)
 *   noInterrupts/interrupts() → global interrupt mask (CLIC mstatus.MIE)
 *   esp_restart()             → software reset via the hardware watchdog
 *
 * The SDK headers below have no extern "C" guards, but this is a C file.
 * FreeRTOS.h (and its platform.h GPIO_REG) is included before Arduino.h so
 * wiring_private.h's `#undef`/redefine of GPIO_REG stays warning-free.
 */
#include <FreeRTOS.h>
#include <cmsis_compatible_gcc.h>
#include <bl_efuse.h>

#include "Arduino.h"

uint32_t esp_get_free_heap_size(void)
{
    return (uint32_t)xPortGetFreeHeapSize();
}

void noInterrupts(void)
{
    __disable_irq();
}

void interrupts(void)
{
    __enable_irq();
}

/* ESP8266-style names (Esp.h's cli()/sei() and Xtensa headers use them). */
void ets_intr_lock(void)
{
    __disable_irq();
}

void ets_intr_unlock(void)
{
    __enable_irq();
}

uint32_t getCpuFrequencyMhz(void)
{
    return 192; /* BL602 BCLK / CPU default */
}

uint32_t esp_get_chip_id(void)
{
    uint8_t mac[6] = {0};

    if (bl_efuse_read_mac(mac) == 0) {
        return (uint32_t)(((uint32_t)mac[0] << 24) | ((uint32_t)mac[1] << 16) |
                          ((uint32_t)mac[2] << 8) | (uint32_t)mac[3]);
    }
    return 0;
}
