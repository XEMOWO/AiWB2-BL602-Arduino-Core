/*
 * umm_impl.cpp — BL602 implementations of the umm_malloc API on newlib.
 *
 * Every allocator call funnels into the standard heap. The heap-select
 * bookkeeping (push/pop/set-by-id) is meaningless with one heap, so those
 * functions return inert values. See umm_malloc_cfg.h.
 */
#include <stdbool.h>
#include <stdlib.h>

#include "umm_malloc/umm_malloc.h"

extern "C" {

void umm_init(void) { /* newlib heap needs no init */ }

void *umm_malloc(size_t size)          { return malloc(size); }
void *umm_calloc(size_t num, size_t size) { return calloc(num, size); }
void *umm_realloc(void *ptr, size_t size) { return realloc(ptr, size); }
void  umm_free(void *ptr)              { free(ptr); }

umm_heap_context_t *umm_push_heap(size_t heap_number) { (void)heap_number; return NULL; }
umm_heap_context_t *umm_pop_heap(void) { return NULL; }
int umm_get_heap_stack_index(void)     { return 0; }
umm_heap_context_t *umm_set_heap_by_id(size_t which) { (void)which; return NULL; }
size_t umm_get_current_heap_id(void)   { return 0; }
umm_heap_context_t *umm_get_current_heap(void) { return NULL; }

void umm_init_iram_ex(void *addr, unsigned int size, bool zero)
{
    (void)addr; (void)size; (void)zero; /* no separate IRAM heap on BL602 */
}

void umm_info(void (*print)(const char *fmt, ...), unsigned int usermem)
{
    (void)print; (void)usermem; /* newlib heap has no per-block stats here */
}

} /* extern "C" */
