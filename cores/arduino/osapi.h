/*
 * osapi.h — ESP8266 non-OS-SDK "os" helper header (BL602 shim).
 *
 * The ESP8266 SDK exposes thin os_* wrappers over its ets_* ROM helpers
 * (os_memcpy, os_sprintf, os_printf, os_timer_*). Third-party code that
 * includes this header gets the same os_* names; on BL602 they map straight to
 * the C library / the core's RISC-V helpers instead of the Xtensa ROM, so the
 * behaviour is equivalent. Kept macro-only (like mem.h) so no extra link
 * symbols appear.
 */
#ifndef _OSAPI_H_
#define _OSAPI_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "esp8266_undocumented.h" /* ets_delay_us, ets_putc, ets_printf, ... */
#include "ets_sys.h"              /* ets_intr_lock/unlock, ets_isr_attach/mask/unmask */

#ifdef __cplusplus
extern "C" {
#endif

#define os_bzero(b, n)         memset((b), 0, (n))
#define os_delay_us(us)        ets_delay_us(us)
#define os_install_putc1       ets_install_putc1
#define os_intr_lock           ets_intr_lock
#define os_intr_unlock         ets_intr_unlock
#define os_isr_attach          ets_isr_attach
#define os_isr_mask            ets_isr_mask
#define os_isr_unmask          ets_isr_unmask
#define os_memcmp              memcmp
#define os_memcpy              memcpy
#define os_memmove             memmove
#define os_memset              memset
#define os_putc                ets_putc
#define os_strcat              strcat
#define os_strchr              strchr
#define os_strcmp              strcmp
#define os_strcpy              strcpy
#define os_strlen              strlen
#define os_strncmp             strncmp
#define os_strncpy             strncpy
#define os_strstr              strstr

#define os_sprintf             sprintf
#define os_snprintf            snprintf
#define os_vsnprintf           vsnprintf

/* os_printf — the SDK prints via ROM; route to the console printf. */
extern int os_printf_plus(const char *format, ...);
#define os_printf              os_printf_plus

/* os_timer_* — the ESP8266 non-OS timers. BL602 code should use Ticker; keep
 * these as no-ops so legacy include sites compile. */
#define os_timer_arm(a, b, c)          ((void)0)
#define os_timer_arm_us(a, b, c)       ((void)0)
#define os_timer_disarm(a)             ((void)0)
#define os_timer_setfn(a, fn, arg)     ((void)0)
#define os_timer_done(a)               ((void)0)
#define os_timer_init(a)               ((void)0)
#define os_timer_handler_isr()         ((void)0)

/* Random — BL602 TRNG. */
unsigned long os_random(void);
int os_get_random(unsigned char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _OSAPI_H_ */
