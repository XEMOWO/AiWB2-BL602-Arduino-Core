/* umm_malloc_cfg.h — BL602 configuration for the ESP8266 umm_malloc API shim.
 *
 * The BL602 runs a single newlib heap; the ESP8266's multi-heap (DRAM / IRAM /
 * external) does not exist here. The umm_malloc API is provided so the
 * memory-inspection examples (HeapMetric, MMU48K, irammem, IramReserve)
 * compile, but every call degrades to the standard malloc/free heap.
 */
#ifndef UMM_MALLOC_CFG_H
#define UMM_MALLOC_CFG_H

#include <stddef.h>
#include <stdbool.h>

/* Heap "context" handle — an opaque id in the upstream allocator. */
typedef unsigned int umm_heap_context_t;

/* Single-heap configuration, matching what the shim implements. */
#define UMM_NUM_HEAPS      1
#define UMM_HEAP_DRAM      1
#define UMM_HEAP_IRAM      2

/* Smallest allocation granularity used by the upstream allocator. */
#define UMM_OVERHEAD_ADJUST 8

#endif /* UMM_MALLOC_CFG_H */
