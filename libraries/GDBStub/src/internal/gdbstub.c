/*
 * gdbstub.c — runtime-harmless GDB-stub for the Ai-WB2-12F (BL602) core.
 *
 * The ESP8266 GDBStub is an Xtensa-specific ROM debugger: it installs a UART0
 * ISR, catches exceptions via the Xtensa EXCVTABLE, and implements the GDB
 * remote protocol. BL602 has no such debug transport, so every entry point is
 * a no-op. The API surface is kept identical so ESP8266 sketches (e.g.
 * gdbstub_example) compile and run — `gdbstub_init()` simply returns, and the
 * hook/override points report "not present" so the core's own UART handling is
 * never disturbed.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "gdb_hooks.h"
#include "GDBStub.h"

void gdbstub_init(void) {}

bool gdbstub_has_putc1_control(void) { return false; }
bool gdbstub_has_uart_isr_control(void) { return false; }

void gdbstub_set_putc1_callback(void (*callback)(char)) { (void)callback; }
void gdbstub_set_uart_isr_callback(void (*callback)(void*, uint8_t), void *arg)
{ (void)callback; (void)arg; }

void gdbstub_write_char(char c) { (void)c; }
void gdbstub_write(const char *buf, size_t size) { (void)buf; (void)size; }

void gdbstub_hook_enable_tx_pin_uart0(uint8_t pin) { (void)pin; }
void gdbstub_hook_enable_rx_pin_uart0(uint8_t pin) { (void)pin; }

/* Overrides of the core's gdb_hooks: nothing to hook up on BL602. */
void gdb_init(void) {}
void gdb_do_break(void) {}
bool gdb_present(void) { return false; }
