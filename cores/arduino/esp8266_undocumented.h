/*
 * esp8266_undocumented.h — ESP8266-core "undocumented" API subset for BL602.
 *
 * The real header (cores/esp8266/esp8266_undocumented.h) exposes the Xtensa
 * SDK's internal ets_* console/delay functions. Examples such as
 * HwdtStackDump/ProcessKey use ets_install_putc1/ets_putc/ets_printf and
 * ets_delay_us. On BL602 the SDK console is newlib printf on UART0; these
 * functions are provided for source compatibility only.
 */
#ifndef ESP8266_UNDOCUMENTED_H
#define ESP8266_UNDOCUMENTED_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Install a character-writer used by ets_printf. The BL602 console already
 * goes to UART0 via newlib, so this only records the callback (harmless). */
typedef void (*putc1_t)(char);
void ets_install_putc1(putc1_t putc1);
void ets_putc(char c);

/* Format to the console (UART0) like printf. */
int ets_printf(const char *fmt, ...);

/* Same, dedicated UART writer (ESP8266 core spells both). */
int ets_uart_printf(const char *fmt, ...);

/* Busy-wait for the given microseconds. */
void ets_delay_us(uint32_t us);

/* osapi.h console/random helpers (BL602 equivalents of the SDK's
 * os_printf_plus/os_random/os_get_random). */
int os_printf_plus(const char *fmt, ...);
unsigned long os_random(void);
int os_get_random(unsigned char *buf, size_t len);

/* Xtensa state-save intrinsics used by bit-bang code (SoftwareSerial, and
 * bare in ProcessKey). On Xtensa, xt_rsil(lvl) returns the previous PS with
 * interrupts masked; xt_wsr_ps() restores a saved PS. BL602 runs bare-metal in
 * machine mode, so the pair maps onto mstatus: clear MIE (bit 3) to mask all
 * interrupts, write it back to restore. (The (void)level mirrors the fact that
 * RISC-V has a single global interrupt enable — any level means "off".) */
#ifndef xt_rsil
static inline uint32_t xt_rsil(int level)
{
    (void)level;
    uint32_t old;
    __asm__ __volatile__("csrrci %0, mstatus, 0x8" : "=r"(old) : : "memory");
    return old;
}
#endif
#ifndef xt_wsr_ps
static inline void xt_wsr_ps(uint32_t state)
{
    __asm__ __volatile__("csrw mstatus, %0" :: "r"(state) : "memory");
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ESP8266_UNDOCUMENTED_H */
