/*
 * esp_compat.c — BL602 implementations of the ESP8266 "undocumented" console
 * helpers and the BearSSL StackThunk no-ops. Kept separate from wiring_*.c so
 * the compatibility surface is easy to audit.
 */
#include <stdarg.h>
#include <stdio.h>

#include "Arduino.h"
#include "esp8266_undocumented.h"
#include "StackThunk.h"
#include "esp8266_peri.h" /* wb2_usart_conf backing USC0() (SerialStress) */
/* Declarations this TU implements (system_* / wifi_* types and the resetInfo
 * extern). Arduino.h only pulls Esp.h under __cplusplus, so include it here
 * explicitly for the C build. */
#include "user_interface.h"

/* Writable storage for the USC0() register shim (see esp8266_peri.h). */
volatile uint32_t wb2_usart_conf[2] = {0, 0};

/* Writable storage for the SPI1 legacy-slave register shim (see
 * esp8266_peri.h, SPISlave library). All zero: the SPISlave ISR stub reads
 * SPIIR, sees no pending bit and does nothing. */
volatile uint32_t wb2_spi1_reg[32] = {0};
volatile uint32_t wb2_spi1_status = 0; /* SPIIR */

/* Writable storage for ESP8266_DREG(addr) (see esp8266_peri.h, MMU48K). */
volatile uint32_t wb2_dreg_shim[256] = {0};

/* lwIP promiscuous capture hook (see user_interface.h). NULL: the BL602 stack
 * has no PHY capture path, so nothing invokes it. */
void (*phy_capture)(int netif_idx, const char* data, size_t len, int out, int success) = NULL;

/* ---- ets_* console helpers ---- */

static putc1_t s_putc1 = NULL;

void ets_install_putc1(putc1_t putc1)
{
    s_putc1 = putc1;
}

void ets_putc(char c)
{
    if (s_putc1) {
        s_putc1(c);
    } else {
        fputc(c, stdout);
    }
}

int ets_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

int ets_uart_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

void ets_delay_us(uint32_t us)
{
    delayMicroseconds(us);
}

/* panic() hook (see debug.h). HwdtStackDump/IramReserve/MMU48K call panic() to
 * force a watchdog reset on demand; on BL602 we print the fault site and stop
 * the world. The HWDT fires if the user has enabled it (Esp.wdtEnable). */
void __attribute__((noreturn)) __panic_func(const char *file, int line, const char *func)
{
    ets_printf("panic: %s:%d in %s\r\n", file, line, func);
    for (;;) {
    }
}

/* osapi.h console/random helpers (see esp8266_undocumented.h). */
int os_printf_plus(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

/* BL602 TRNG word, as in Esp.h (bl_sec_get_random_word). Declared here for the
 * C build; implemented by the SDK's secure-engine driver (libbl602_sec_eng or
 * the bl602_std glue in libs). */
extern uint32_t bl_sec_get_random_word(void);

/* os_random/os_get_random are also provided (strongly) by the SDK's
 * libwpa_supplicant.a os_bl.c (bl_rand()). Mark ours weak so the SDK's wins
 * when that archive is pulled in; a sketch that references them without the
 * SDK stack still links against these. */
__attribute__((weak)) unsigned long os_random(void)
{
    return (unsigned long)bl_sec_get_random_word();
}

__attribute__((weak)) int os_get_random(unsigned char *buf, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        buf[i] = (unsigned char)(bl_sec_get_random_word() >> ((i & 3) * 8));
    }
    return 0;
}

/* MMU48K example: `extern "C" void _text_end(void)` from the ESP8266 linker
 * script, cast to an address to print the end of the IRAM/code region. BL602's
 * linker script has no _text_end; provide a weak dummy so the sketch compiles
 * and links (the printed value is the address of this stub). */
__attribute__((weak)) void _text_end(void)
{
}

/* StaticLease example walks the ESP8266 non-OS-SDK connected-station list.
 * BL602 keeps stations in wifi_mgmr; report an empty list (NULL head) so the
 * walk sees no clients — compile-compatible and harmless. */
struct station_info *wifi_softap_get_station_info(void)
{
    return NULL;
}

void wifi_softap_free_station_list(void)
{
}

/* ---- StackThunk no-ops ---- */

uint32_t *stack_thunk_ptr  = NULL;
uint32_t *stack_thunk_top  = NULL;
uint32_t *stack_thunk_save = NULL;
uint32_t  stack_thunk_refcnt = 0;

void stack_thunk_add_ref(void) { stack_thunk_refcnt++; }
void stack_thunk_del_ref(void) { if (stack_thunk_refcnt) stack_thunk_refcnt--; }
void stack_thunk_repaint(void) { /* no-op */ }

uint32_t stack_thunk_get_refcnt(void)     { return stack_thunk_refcnt; }
uint32_t stack_thunk_get_stack_top(void)  { return 0; }
uint32_t stack_thunk_get_stack_bot(void)  { return 0; }
uint32_t stack_thunk_get_cont_sp(void)    { return 0; }
uint32_t stack_thunk_get_max_usage(void)  { return 0; }
void stack_thunk_dump_stack(void)         { /* no-op */ }
void stack_thunk_fatal_overflow(void)     { /* no-op */ }

/* ---- non-OS-SDK system introspection (TestEspApi, interactive) ----
 *
 * Pure system queries live here (no WiFi state needed). BL602 exposes no
 * boot-version/userbin concept; those report 0. The flash-size map reports
 * the 2 MB part as the closest ESP8266 map. WiFi radio calls (wifi_set_opmode
 * & friends) are implemented in the ESP8266WiFi library where the state is.
 */
struct rst_info* system_get_rst_info(void)      { return &resetInfo; }
uint32 system_get_time(void)                    { return (uint32)millis(); }
void system_print_meminfo(void)                 { /* no-op */ }
uint32 system_get_free_heap_size(void)          { return esp_get_free_heap_size(); }
void system_set_os_print(uint8 onoff)           { (void)onoff; /* console always on */ }
uint8 system_get_os_print(void)                 { return 0; }
uint32 system_get_chip_id(void)                 { return esp_get_chip_id(); }
const char *system_get_sdk_version(void)        { return "bl602-sdk"; }
uint8 system_get_boot_version(void)             { return 0; }
uint32 system_get_userbin_addr(void)            { return 0; }
uint8 system_get_boot_mode(void)                { return 1; } /* SYS_BOOT_NORMAL_MODE */
bool system_update_cpu_freq(uint8 freq)         { (void)freq; return true; } /* fixed 192 MHz */
uint8 system_get_cpu_freq(void)                 { return (uint8)getCpuFrequencyMhz(); }
enum flash_size_map system_get_flash_size_map(void)
{
    return FLASH_SIZE_16M_MAP_512_512; /* closest ESP8266 map for a 2 MB part */
}

/* ---- flash layout markers (ESP8266httpUpdate) ----
 * ESP8266httpUpdate.cpp computes the SPIFFS partition size as the span between
 * the linker-script symbols _FS_start and _FS_end (see FS.h). BL602 has no
 * such embedded partition, so both are zero-length: spiffs updates degrade to
 * a no-op while the sketch still links unchanged. */
uint32_t _FS_start = 0;
uint32_t _FS_end   = 0;
