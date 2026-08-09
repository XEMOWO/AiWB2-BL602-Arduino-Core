/*
 * pgmspace.h — PROGMEM / F() compatibility for the Ai-WB2-12F core.
 *
 * BL602 executes from flash through an XIP/cache path and the linker already
 * places `const` data in flash, so PROGMEM is a no-op: `const` arrays are
 * readable directly. The macros exist so sketches written for AVR/ESP cores
 * compile unchanged.
 */
#ifndef pgmspace_h
#define pgmspace_h

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PROGMEM
#define PSTR(s) (s)
#define F(x)    (x)
#define fstring_t const char *

#define pgm_read_byte(a)      (*(const uint8_t *)(a))
#define pgm_read_word(a)      (*(const uint16_t *)(a))
#define pgm_read_dword(a)     (*(const uint32_t *)(a))
#define pgm_read_float(a)     (*(const float *)(a))
#define pgm_read_ptr(a)       (*(const void **)(a))

#define strlen_P(a)           strlen((a))
#define strcpy_P(d, s)        strcpy((d), (s))
#define strncpy_P(d, s, n)    strncpy((d), (s), (n))
#define strcmp_P(a, b)        strcmp((a), (b))
#define strncmp_P(a, b, n)    strncmp((a), (b), (n))
#define strstr_P(a, b)        strstr((a), (b))
#define strchr_P(a, b)        strchr((a), (b))
#define memcpy_P(d, s, n)     memcpy((d), (s), (n))
#define memcmp_P(a, b, n)     memcmp((a), (b), (n))

#endif /* pgmspace_h */
