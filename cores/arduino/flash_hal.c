/*
 * flash_hal.c — flash_hal_read/write/erase for BL602.
 *
 * The ESP8266 flash_hal_* API works on raw flash offsets (0 = start of the
 * flash part). BL602 exposes the same thing through the SDK's HOSAL raw-flash
 * interface (hosal_flash_raw_read/write/erase), which is what the SDK's own
 * SPIFFS driver (bl_spiffs.c) uses. Return codes mirror the ESP8266 header so
 * LittleFS (and any third-party code) behaves identically.
 */
#include "flash_hal.h"

#include <hosal_flash.h>

int32_t flash_hal_read(uint32_t addr, uint32_t size, uint8_t *dst)
{
    return hosal_flash_raw_read(dst, addr, size) == 0 ? FLASH_HAL_OK : FLASH_HAL_READ_ERROR;
}

int32_t flash_hal_write(uint32_t addr, uint32_t size, const uint8_t *src)
{
    return hosal_flash_raw_write((void *)src, addr, size) == 0 ? FLASH_HAL_OK : FLASH_HAL_WRITE_ERROR;
}

int32_t flash_hal_erase(uint32_t addr, uint32_t size)
{
    return hosal_flash_raw_erase(addr, size) == 0 ? FLASH_HAL_OK : FLASH_HAL_ERASE_ERROR;
}
