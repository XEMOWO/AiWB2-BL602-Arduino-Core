/*
 * mmu_iram.h — ESP8266 MMU / XIP flash byte-access helpers (BL602 shim).
 *
 * On the Xtensa ESP8266, code and constants are pulled from flash via the SPI
 * MMU (memory-management unit), and these helpers read/write through it with
 * explicit widths. BL602 reads the SPI flash directly through XIP, so the same
 * accesses are plain volatile memory reads — the macros below are defined so
 * code written for the ESP8266 (irammem, virtualmem) compiles unchanged.
 */
#ifndef MMU_IRAM_H
#define MMU_IRAM_H

#include <stdint.h>
/* Same as the ESP8266 core: mmu_iram.h brings in the ets_* console helpers
 * (irammem uses ETS_PRINTF = ets_uart_printf). */
#include "esp8266_undocumented.h"

/* XIP address-space constant (ESP8266 SPI flash base). Only used by sketches
 * that hard-code the address map; the WB2 flash is at a different base. */
#define VIRTUALMEM_FLASH_BASE 0x40200000

#define mmu_get_uint8(p)    (*(volatile uint8_t *)(p))
#define mmu_set_uint8(p, v) ((*(volatile uint8_t *)(p)) = (uint8_t)(v))
#define mmu_get_int8(p)     (*(volatile int8_t *)(p))
#define mmu_set_int8(p, v)  ((*(volatile int8_t *)(p)) = (int8_t)(v))
#define mmu_get_uint16(p)   (*(volatile uint16_t *)(p))
#define mmu_set_uint16(p, v)((*(volatile uint16_t *)(p)) = (uint16_t)(v))
#define mmu_get_int16(p)    (*(volatile int16_t *)(p))
#define mmu_set_int16(p, v) ((*(volatile int16_t *)(p)) = (int16_t)(v))
#define mmu_get_uint32(p)   (*(volatile uint32_t *)(p))
#define mmu_set_uint32(p, v)((*(volatile uint32_t *)(p)) = (uint32_t)(v))
#define mmu_get_int32(p)    (*(volatile int32_t *)(p))
#define mmu_set_int32(p, v) ((*(volatile int32_t *)(p)) = (int32_t)(v))

#endif /* MMU_IRAM_H */
