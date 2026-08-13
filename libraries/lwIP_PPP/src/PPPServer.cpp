/*
 * PPPServer.cpp — runtime-harmless PPP server for the Ai-WB2-12F (BL602) core.
 *
 * The ESP8266 PPPServer drives lwIP's pppos (PPP over serial) through a
 * Stream: it creates a ppp_pcb, negotiates IPCP over a UART, and NATs the
 * connected host onto the STA link. BL602's lwIP has no PPP/PPPoS compiled in
 * and no second usable UART pair free for a link, so `begin()` simply reports
 * failure and nothing is ever created. The full ESP8266 API surface is kept so
 * PPPServer examples compile and run — they log "ppp: 0" and idle.
 */
#include <Arduino.h>
#include <IPAddress.h>
#include "PPPServer.h"

PPPServer::PPPServer(Stream* sio) : _sio(sio), _cb(netif_status_cb_s), _enabled(false) {}

bool PPPServer::handlePackets()
{
    return _enabled;
}

u32_t PPPServer::output_cb_s(ppp_pcb* pcb, u8_t* data, u32_t len, void* ctx)
{
    (void)pcb; (void)data; (void)len; (void)ctx;
    return 0;
}

void PPPServer::link_status_cb_s(ppp_pcb* pcb, int err_code, void* ctx)
{
    (void)pcb; (void)err_code; (void)ctx;
}

void PPPServer::netif_status_cb_s(netif* nif)
{
    (void)nif;
}

bool PPPServer::begin(const IPAddress& ourAddress, const IPAddress& peer)
{
    (void)ourAddress; (void)peer;
    /* No PPPoX in the BL602 lwIP build: never open a link. */
    _enabled = false;
    return false;
}

void PPPServer::stop()
{
    _enabled = false;
}
