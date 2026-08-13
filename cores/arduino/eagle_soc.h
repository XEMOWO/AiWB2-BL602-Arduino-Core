/*
 * eagle_soc.h — ESP8266 SoC register shim for the Ai-WB2-12F (BL602) core.
 *
 * ESP8266 third-party libraries (notably Adafruit_NeoPixel's esp8266.c) include
 * <eagle_soc.h> and poke the GPIO "W1TS/W1TC" set/clear registers to bit-bang
 * timing-critical protocols. BL602's GPIO lives in the GLB block as a plain
 * R/W register (wiring_private.h), so:
 *   - GPIO_OUT_W1TS_ADDRESS / GPIO_OUT_W1TC_ADDRESS are symbolic addresses that
 *     GPIO_REG_WRITE() dispatches to a read-modify-write of GLB GPIO_OUTPUT
 *     (set / clear), constant-folded at compile time;
 *   - _BV() is the usual bit macro;
 *   - on RISC-V, the one Xtensa instruction NeoPixel embeds (rsr ccount) is
 *     neutralised and the cycle count comes from the RISC-V `cycle` CSR.
 */
#ifndef EAGLE_SOC_H
#define EAGLE_SOC_H

#include <stdint.h>
#include "wiring_private.h"   /* GPIO_REG + GLB GPIO register offsets */

/* ---- GPIO set/clear register emulation (see header comment) ---- */
#define GPIO_OUT_W1TS_ADDRESS  0xE0000188UL   /* symbolic "set"   register */
#define GPIO_OUT_W1TC_ADDRESS  0xE000018CUL   /* symbolic "clear" register */

#define GPIO_REG_WRITE(addr, val) \
    ((addr) == GPIO_OUT_W1TS_ADDRESS ? \
        (GPIO_REG(GPIO_OUTPUT_VAL) |= (uint32_t)(val)) : \
        (GPIO_REG(GPIO_OUTPUT_VAL) &= ~(uint32_t)(val)))

#define _BV(x) (1UL << (x))

/* ---- Xtensa-only asm neutraliser (RISC-V only) ---- */
#if defined(ESP8266) && defined(__riscv)
#ifndef WB2_EAGLE_SOC_PATCHED
#define WB2_EAGLE_SOC_PATCHED 1

static inline uint32_t wb2_read_cycle(void)
{
    uint32_t c;
    __asm__ volatile ("csrr %0, cycle" : "=r"(c));
    return c;
}

/* NeoPixel's esp8266.c contains exactly one Xtensa instruction:
 *     __asm__ __volatile__("rsr %0,ccount":"=a"(ccount));
 * It cannot assemble on RISC-V. Swallow the asm statement and redirect the
 * `ccount` identifier to the RISC-V cycle CSR. Scope: only TUs that include
 * <eagle_soc.h> on a RISC-V build (in practice esp8266.c). */
#define __asm__
#define __volatile__ wb2_eat_asm
#define wb2_eat_asm(...) /* swallow */
#define ccount wb2_cc_var = wb2_read_cycle()

#endif /* WB2_EAGLE_SOC_PATCHED */
#endif /* ESP8266 && __riscv */

#endif /* EAGLE_SOC_H */
