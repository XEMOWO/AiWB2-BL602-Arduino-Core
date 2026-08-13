/*
 * mem.h — ESP8266 non-OS-SDK memory helpers (BL602 shim).
 *
 * The SDK header aliases the ESP8266 os_* allocators to the C library. On
 * BL602 the C library allocators are used directly, so this file is a thin
 * pass-through: every os_* name maps to its plain <stdlib.h> counterpart so
 * third-party code (ESP8266AVRISP) compiles unchanged.
 */
#ifndef __MEM_H__
#define __MEM_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define os_free     free
#define os_malloc   malloc
#define os_calloc   calloc
#define os_realloc  realloc
#define os_zalloc(s) calloc(1, (s))
#define zalloc(s)    calloc(1, (s))

#define MEMLEAK_DEBUG_ENABLE 0

#ifdef __cplusplus
}
#endif

#endif /* __MEM_H__ */
