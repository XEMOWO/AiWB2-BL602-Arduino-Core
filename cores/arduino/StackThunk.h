/*
 * StackThunk.h — ESP8266 BearSSL secondary-stack thunking (BL602 shim).
 *
 * On the Xtensa ESP8266, BearSSL's large stack frames overflowed the default
 * stack, so the core added a "thunk" mechanism that swaps to a user heap
 * stack around each call (implemented as Xtensa inline asm). RISC-V has no
 * such mechanism and the BL602 uses a larger stack anyway, so these are
 * compile-compat no-ops: the API (stack_thunk_add_ref/del_ref, the
 * make_stack_thunk macro) exists so sketches like HwdtStackDump still build,
 * but nothing is actually thunked.
 */
#ifndef _STACKTHUNK_H
#define _STACKTHUNK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reference-counted thunk stack lifetime (no-ops on BL602). */
void stack_thunk_add_ref(void);
void stack_thunk_del_ref(void);
void stack_thunk_repaint(void);

uint32_t stack_thunk_get_refcnt(void);
uint32_t stack_thunk_get_stack_top(void);
uint32_t stack_thunk_get_stack_bot(void);
uint32_t stack_thunk_get_cont_sp(void);
uint32_t stack_thunk_get_max_usage(void);
void stack_thunk_dump_stack(void);
void stack_thunk_fatal_overflow(void);

/* Globals required for thunking operation (present for source compat). */
extern uint32_t *stack_thunk_ptr;
extern uint32_t *stack_thunk_top;
extern uint32_t *stack_thunk_save;
extern uint32_t stack_thunk_refcnt;

/* The Xtensa macro defined `thunk_<fcn>` as an asm trampoline that swaps onto
 * the secondary stack and calls <fcn>. RISC-V has no such mechanism, so alias
 * `thunk_<fcn>` to <fcn> itself: calls to thunk_<fcn>(...) resolve straight to
 * the real function with no stack juggling. The thunked functions in the
 * ESP8266 examples are printf-style, hence the fixed signature. */
#define make_stack_thunk(fcnToThunk) \
    extern "C" int thunk_##fcnToThunk(const char *fmt, ...) __attribute__((alias(#fcnToThunk)))

#ifdef __cplusplus
}
#endif

#endif /* _STACKTHUNK_H */
