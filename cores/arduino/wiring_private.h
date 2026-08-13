/*
 * wiring_private.h — low-level helpers shared by core and bit-bang libraries.
 *
 * Included from Arduino.h so that third-party libraries that do direct GPIO
 * register access (e.g. OneWire) find the same symbol set as on other cores.
 *
 * RISC-V direct-gpio libraries were written against the SiFive HiFive1 GPIO
 * block: they read/write a flat bit-per-pin register space through
 * GPIO_REG(GPIO_*). BL602's GPIO lives in the GLB block with the same
 * bit-per-pin layout for input value / output value / output enable:
 *
 *     GLB_BASE(0x40000000) + 0x180 = GPIO_INPUT   (read pad level)
 *                           + 0x188 = GPIO_OUTPUT  (write drive level)
 *                           + 0x190 = GPIO_OUTPUT_EN
 *
 * INPUT_EN / IOF_EN / OUTPUT_XOR have no BL602 aggregate register (pin mux and
 * input enable are per-pin CFGCTL fields, set once by pinMode()). OneWire only
 * ever clears/sets them paired with OUTPUT_EN inside one critical section, and
 * the final state then equals the OUTPUT_EN write - so routing all three to the
 * OUTPUT_EN register yields the correct end state for that access pattern.
 */
#ifndef wiring_private_h
#define wiring_private_h

#include <stdint.h>

/* The SDK's FreeRTOS port (freertos_riscv_ram/config/platform.h) defines a
 * GPIO_REG() for a legacy SiFive-style GPIO block at 0x10012000 that NO BL602
 * driver uses. Undef so our GLB mapping below is authoritative whenever this
 * header is included. */
#ifdef GPIO_REG
#undef GPIO_REG
#endif
#define GPIO_REG(reg)        (*(volatile uint32_t *)(0x40000000UL + (reg)))

#define GPIO_INPUT_VAL       0x180UL   /* read pad level, bit = GPIO number */
#define GPIO_OUTPUT_VAL      0x188UL   /* write drive level */
#define GPIO_OUTPUT_EN       0x190UL   /* aggregate output enable */
#define GPIO_INPUT_EN        GPIO_OUTPUT_EN   /* no aggregate reg; see header */
#define GPIO_IOF_EN          GPIO_OUTPUT_EN
#define GPIO_OUTPUT_XOR      GPIO_OUTPUT_EN

#endif /* wiring_private_h */
