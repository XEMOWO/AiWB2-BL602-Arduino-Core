/*
 flash_hal.h - flash hal interface (ESP8266-compatible, BL602 shim).
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 ...
 This file is part of the esp8266 core for Arduino environment.

 Ported to Ai-WB2-12F (BL602): the ESP8266 flash_hal_* functions operate on
 raw flash offsets (0 = start of the flash part). Implemented in flash_hal.c
 on top of the SDK's hosal_flash_raw_* calls, so the values match. The
 ESP8266 FS_PHYS_ADDR/SIZE/PAGE/BLOCK macros that normally live here are
 resolved at runtime from the boot2 partition table instead (see LittleFS),
 so they are intentionally omitted.
 */
#ifndef FLASH_HAL_H
#define FLASH_HAL_H

#include <stdint.h>

#define FLASH_HAL_OK          (0)
#define FLASH_HAL_READ_ERROR  (-1)
#define FLASH_HAL_WRITE_ERROR (-2)
#define FLASH_HAL_ERASE_ERROR (-3)

#ifdef __cplusplus
extern "C" {
#endif

int32_t flash_hal_write(uint32_t addr, uint32_t size, const uint8_t *src);
int32_t flash_hal_erase(uint32_t addr, uint32_t size);
int32_t flash_hal_read(uint32_t addr, uint32_t size, uint8_t *dst);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_HAL_H */
