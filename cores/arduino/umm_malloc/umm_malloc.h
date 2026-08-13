/*
 * umm_malloc.h — ESP8266-compatible memory-allocator interface (BL602 shim).
 *
 * The ESP8266 core builds its heap on the umm_malloc library and exposes the
 * allocator to sketches via this header. BL602 uses the newlib heap, so this
 * declares the same API (implemented in umm_impl.cpp on top of malloc/free).
 * Mirrors the declaration list from cores/esp8266/umm_malloc/umm_malloc.h.
 */
#ifndef UMM_MALLOC_H
#define UMM_MALLOC_H

#include <stdint.h>
#include <stddef.h>

#include "umm_malloc_cfg.h" /* UMM_NUM_HEAPS / UMM_HEAP_IRAM / ... */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UMM_HEAP_IRAM
/* IRAM heap init. umm_init_iram() is defined by the IramReserve example
 * itself; only umm_init_iram_ex() is provided by the core shim. */
extern void umm_init_iram(void);
extern void umm_init_iram_ex(void *addr, unsigned int size, bool zero);
#endif

extern void  umm_init(void);
extern void *umm_malloc(size_t size);
extern void *umm_calloc(size_t num, size_t size);
extern void *umm_realloc(void *ptr, size_t size);
extern void  umm_free(void *ptr);

extern umm_heap_context_t *umm_push_heap(size_t heap_number);
extern umm_heap_context_t *umm_pop_heap(void);
extern int umm_get_heap_stack_index(void);
extern umm_heap_context_t *umm_set_heap_by_id(size_t which);
extern size_t umm_get_current_heap_id(void);
extern umm_heap_context_t *umm_get_current_heap(void);

/* Dump heap stats; print is NULL to use stdout. */
extern void umm_info(void (*print)(const char *fmt, ...), unsigned int usermem);

#ifdef __cplusplus
}
#endif

#endif /* UMM_MALLOC_H */
