/*
 * md5.h — MD5 digest, API-compatible with cores/esp8266/md5.h.
 *
 * The ESP8266 core exposes the ROM's MD5 via MD5Init/MD5Update/MD5Final.
 * This is a self-contained reimplementation (RFC 1321) so MD5Builder and any
 * third-party code including <md5.h> links on BL602 without mbedtls.
 *
 * Original C source: https://github.com/morrissinger/ESP8266-Websocket (MD5.h)
 */
#ifndef __ESP8266_MD5__
#define __ESP8266_MD5__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t  buffer[64];
} md5_context_t;

extern void MD5Init(md5_context_t *);
extern void MD5Update(md5_context_t *, const uint8_t *, const uint16_t);
extern void MD5Final(uint8_t[16], md5_context_t *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __ESP8266_MD5__ */
