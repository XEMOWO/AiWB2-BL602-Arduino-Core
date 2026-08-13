/*
 * spi_flash.h — ESP8266-core compatible SPI-flash API for the BL602.
 *
 * The ESP8266 core exposes spi_flash_read/write/erase_sector on top of its
 * ROM SPI-flash driver, and examples (CheckFlashCRC) include this header
 * directly. BL602's equivalent is the hosal flash layer (bl_flash_*), whose
 * calls already take byte addresses into the flash.
 *
 * NOTE: ESP8266 examples that reach into flash compute addresses from the
 * Xtensa XIP base 0x40200000 — that mapping does not exist on BL602, so such
 * examples compile but must not be run unmodified (they would touch arbitrary
 * flash). The wrappers below keep the SPI_FLASH_* API so the code still builds.
 */
#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>

/* The SDK's bl_flash.h has no extern "C" guard, but its symbols are C linkage
 * (libhosal). Without the guard, C++ TUs (sketches, core .cpp) emit mangled
 * references (_Z14bl_flash_eraseji) that never match the archive's symbols.
 * Keep the include inside extern "C" so every caller gets the C name. */
#ifdef __cplusplus
extern "C" {
#endif
#include "bl_flash.h" /* bl_flash_read/write/erase (libhosal) */
#ifdef __cplusplus
}
#endif

/* 4 KiB standard SPI NOR sector. Same value as the ESP8266 core. */
#define SPI_FLASH_SEC_SIZE       4096
#define SPI_FLASH_SECTOR_SIZE    SPI_FLASH_SEC_SIZE

/* ESP8266-compatible result codes (spi_flash_op_result). */
#define SPI_FLASH_RESULT_OK       0
#define SPI_FLASH_RESULT_ERR      1
#define SPI_FLASH_RESULT_TIMEOUT  2

/* The ESP8266 spi_flash_* functions take offsets into the flash array;
 * bl_flash_* already consumes the same address space on BL602. */
static inline int spi_flash_read(uint32_t offset, uint32_t *data, uint32_t size)
{
    return bl_flash_read(offset, (uint8_t *)data, (int)size);
}

static inline int spi_flash_write(uint32_t offset, uint32_t *data, uint32_t size)
{
    return bl_flash_write(offset, (uint8_t *)data, (int)size);
}

static inline int spi_flash_erase_sector(uint16_t sector)
{
    return bl_flash_erase((uint32_t)sector * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE);
}

static inline int spi_flash_erase_block(uint32_t block)
{
    return bl_flash_erase(block * 64 * SPI_FLASH_SEC_SIZE, 64 * SPI_FLASH_SEC_SIZE);
}

#endif /* SPI_FLASH_H */
