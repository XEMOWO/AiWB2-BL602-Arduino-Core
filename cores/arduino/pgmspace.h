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
#include <stdio.h>

#define PROGMEM
#define PSTR(s) (s)
#define fstring_t const char *

/* The official F() macro yields a const __FlashStringHelper* so that code
 * which passes F("...") to a `const __FlashStringHelper*` parameter (e.g. many
 * RTC/display libraries) compiles. Because PROGMEM is a no-op here the pointer
 * really is a const char*; Print's overloads cast it straight back. */
#ifdef __cplusplus
class __FlashStringHelper;
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(x) (FPSTR(PSTR(x)))
#else
#define FPSTR(pstr_pointer) (pstr_pointer)
#define F(x) (x)
#endif

/* AVR/ESP pointer-to-flash typedefs (readable directly on XIP). */
typedef const char  *PGM_P;
typedef const void  *PGM_VOID_P;
typedef const char  *PGMStr;

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
#define strrchr_P(a, b)       strrchr((a), (b))
#define memcpy_P(d, s, n)     memcpy((d), (s), (n))
#define memcmp_P(a, b, n)     memcmp((a), (b), (n))

/* The *printf_P family. PSTR()/F() produce const __FlashStringHelper* here,
 * but PROGMEM is a no-op so it is really a const char* — cast before calling
 * the libc formatter (the GNU ##__VA_ARGS__ swallows the trailing comma when
 * no extra arguments are supplied). */
#define sprintf_P(s, f, ...)       sprintf((s), (const char *)(f), ##__VA_ARGS__)
#define snprintf_P(s, f, n, ...)   snprintf((s), (n), (const char *)(f), ##__VA_ARGS__)
#define vsnprintf_P(s, f, n, a)    vsnprintf((s), (n), (const char *)(f), (a))
#define sscanf_P(s, f, ...)        sscanf((s), (const char *)(f), ##__VA_ARGS__)

#endif /* pgmspace_h */
