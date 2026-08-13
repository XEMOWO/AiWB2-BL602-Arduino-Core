/*
 * compat_stdint_fix.h — re-point the rv32 32-bit integer base types to match
 * the Xtensa ESP8266, where int32_t/uint32_t are int/unsigned-int.
 *
 * WHY: this rv32 GCC is a riscv64-elf compiler driving the rv32 multilib, and
 * it defines __INT32_TYPE__/__UINT32_TYPE__ as long/long-unsigned. The Xtensa
 * ESP8266 defines them as int/unsigned-int, and ESP8266 code — function
 * signatures (cyclesToRead1Kx32(unsigned int*, ...)), %u/%d printf of uint32_t,
 * casts between uint32_t* and unsigned int* — relies on that identity.
 *
 * HOW: GNU C lets an ordinary header #undef/#define the compiler's built-in
 * type macros before <stdint.h> is reached. newlib's stdint.h then builds
 * int32_t/uint32_t/uint_least32_t from our values. All four types stay 32-bit,
 * so the SDK's prebuilt libraries (compiled with the defaults) remain
 * ABI-compatible — the change is in type spelling only.
 *
 * This header is force-included via `-include ...` from platform.txt so every
 * translation unit (core, libraries, sketches) sees identical type identity.
 */
#ifndef COMPAT_STDINT_FIX_H
#define COMPAT_STDINT_FIX_H

#undef  __INT32_TYPE__
#define __INT32_TYPE__ int
#undef  __UINT32_TYPE__
#define __UINT32_TYPE__ unsigned int

#undef  __INT_LEAST32_TYPE__
#define __INT_LEAST32_TYPE__ int
#undef  __UINT_LEAST32_TYPE__
#define __UINT_LEAST32_TYPE__ unsigned int

#endif /* COMPAT_STDINT_FIX_H */
