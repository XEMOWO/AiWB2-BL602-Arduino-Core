/*
 * gpio.h — ESP8266 SDK <gpio.h> compatibility shim for the Ai-WB2-12F (BL602) core.
 *
 * BL602 GPIO is a plain R/W data register in the GLB block (wiring_private.h);
 * there are no per-pin interrupt-shadow registers like the ESP8266's. The SDK
 * API is kept for compile compatibility: the set/clear/read macros go straight
 * to the GLB GPIO data registers (bit = GPIO number, matching pinMode()'s
 * IOMUX setup), and the interrupt / wakeup helpers are harmless no-ops.
 */
#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPIO_PIN_INTR_DISABLE = 0,
    GPIO_PIN_INTR_POSEDGE,
    GPIO_PIN_INTR_NEGEDGE,
    GPIO_PIN_INTR_ANYEDGE,
    GPIO_PIN_INTR_LOLEVEL,
    GPIO_PIN_INTR_HILEVEL,
} GPIO_INT_TYPE;

typedef void (*gpio_intr_handler_fn_t)(uint32_t intr_mask, void *arg);

/* ---- BL602 GLB GPIO data registers (same offsets wiring_private.h uses) ---- */
#define GPIO_OUTPUT_DATA  (*(volatile uint32_t *)(0x40000000UL + 0x188UL))
#define GPIO_INPUT_DATA   (*(volatile uint32_t *)(0x40000000UL + 0x180UL))

#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
    (GPIO_OUTPUT_DATA = (GPIO_OUTPUT_DATA & ~(1UL << (gpio_no))) | ((uint32_t)(bit_value) << (gpio_no)))
#define GPIO_DIS_OUTPUT(gpio_no) (GPIO_OUTPUT_DATA &= ~(1UL << (gpio_no)))
#define GPIO_INPUT_GET(gpio_no)  ((GPIO_INPUT_DATA >> (gpio_no)) & 1UL)

static inline void gpio_init(void) {}

static inline void gpio_output_set(uint32_t set_mask, uint32_t clear_mask,
                                   uint32_t enable_mask, uint32_t disable_mask)
{
    GPIO_OUTPUT_DATA = (GPIO_OUTPUT_DATA & ~disable_mask & ~clear_mask) | set_mask;
    (void)enable_mask;
}

static inline uint32_t gpio_input_get(void) { return GPIO_INPUT_DATA; }

static inline void gpio_register_set(uint32_t reg_id, uint32_t value) { (void)reg_id; (void)value; }
static inline uint32_t gpio_register_get(uint32_t reg_id) { (void)reg_id; return 0; }

static inline void gpio_intr_handler_register(gpio_intr_handler_fn_t fn, void *arg) { (void)fn; (void)arg; }
static inline uint32_t gpio_intr_pending(void) { return 0; }
static inline void gpio_intr_ack(uint32_t ack_mask) { (void)ack_mask; }
static inline void gpio_pin_wakeup_enable(uint32_t i, GPIO_INT_TYPE intr_state) { (void)i; (void)intr_state; }
static inline void gpio_pin_wakeup_disable(void) {}
static inline void gpio_pin_intr_state_set(uint32_t i, GPIO_INT_TYPE intr_state) { (void)i; (void)intr_state; }

#ifdef __cplusplus
}
#endif

#endif /* _GPIO_H_ */
