/* lwip/napt.h — Network Address and Port Translation API (WB2 stub).
 *
 * The reference ESP8266 SDK ships a real lwIP NAPT implementation and the
 * ESP8266 core builds with IP_FORWARD/IP_NAPT enabled, so sketches like
 * RangeExtender-NAPT and NAPTCaptivePortal call ip_napt_init() /
 * ip_napt_enable_no(). The BL602 SDK's lwIP has no NAPT and does not forward
 * packets, so this header declares the same API as harmless static-inline
 * no-ops: every ESP8266 sketch that pokes at NAT compiles, and at runtime the
 * calls succeed (ERR_OK) while the IP stack simply never NATs.
 *
 * This file is a drop-in for the lwip/ include directory (the SDK's real
 * lwip/ dir has no napt.h, so the compiler finds this one via the core's own
 * include path). The types (err_t/u16_t/...) come from the SDK's lwIP.
 */
#ifndef __LWIP_NAPT_H__
#define __LWIP_NAPT_H__

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/prot/ip.h" /* IP_PROTO_TCP etc. */

#ifdef __cplusplus
extern "C" {
#endif

#if LWIP_IPV6
#error "napt.h is not supported with LWIP_IPV6"   /* mirrors the ESP8266 guard */
#endif

/** Allocate and initialize the NAPT tables (no-op on WB2). */
static inline err_t
ip_napt_init(u16_t max_nat, u8_t max_portmap)
{
    (void)max_nat;
    (void)max_portmap;
    return ERR_OK;
}

/** Enable/Disable NAPT for a specified interface (no-op on WB2). */
static inline err_t
ip_napt_enable_no(u8_t number, int enable)
{
    (void)number;
    (void)enable;
    return ERR_OK;
}

/** Enable/Disable NAPT for a specified interface address (no-op on WB2). */
static inline err_t
ip_napt_enable(u32_t addr, int enable)
{
    (void)addr;
    (void)enable;
    return ERR_OK;
}

/** Register an external→internal port mapping (no-op on WB2). */
static inline err_t
ip_portmap_add(u8_t proto, u32_t maddr, u16_t mport, u32_t daddr, u16_t dport)
{
    (void)proto; (void)maddr; (void)mport; (void)daddr; (void)dport;
    return ERR_OK;
}

/** Unregister a port mapping (no-op on WB2). */
static inline err_t
ip_portmap_remove(u8_t proto, u32_t maddr, u16_t mport)
{
    (void)proto; (void)maddr; (void)mport;
    return ERR_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* __LWIP_NAPT_H__ */
