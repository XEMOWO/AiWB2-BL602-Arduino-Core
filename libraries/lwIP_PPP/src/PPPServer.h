#ifndef __PPPSERVER_H
#define __PPPSERVER_H

/*
 * PPPServer — compile-compatible PPP server for the Ai-WB2-12F (BL602) core.
 *
 * The reference ESP8266 header pulls <netif/ppp/ppp.h> + <netif/ppp/pppos.h>;
 * the BL602 lwIP is built with PPP_SUPPORT=0, so those headers declare nothing.
 * `struct ppp_pcb` is forward-declared here instead — the class only ever
 * stores a pointer to it (the no-op implementation never touches PPP symbols).
 * The public API is byte-for-byte the ESP8266 one, so PPPServer examples
 * compile unchanged.
 */
#include <Arduino.h>
#include <IPAddress.h>
#include <lwip/netif.h>

struct ppp_pcb;  /* lwIP PPP control block (PPP_SUPPORT off on BL602) */

class PPPServer
{
public:
    PPPServer(Stream* sio);

    bool begin(const IPAddress& ourAddress, const IPAddress& peer = IPAddress(172, 31, 255, 254));
    void stop();

    void ifUpCb(void (*cb)(netif*))
    {
        _cb = cb;
    }
    const ip_addr_t* getPeerAddress() const
    {
        return &_netif.gw;
    }

protected:
    static constexpr size_t _bufsize = 128;
    Stream*                 _sio;
    ppp_pcb*                _ppp;
    netif                   _netif;
    void (*_cb)(netif*);
    uint8_t _buf[_bufsize];
    bool    _enabled;

    // feed ppp from stream - to call on a regular basis or on interrupt
    bool handlePackets();

    static u32_t output_cb_s(ppp_pcb* pcb, u8_t* data, u32_t len, void* ctx);
    static void  link_status_cb_s(ppp_pcb* pcb, int err_code, void* ctx);
    static void  netif_status_cb_s(netif* nif);
};

#endif  // __PPPSERVER_H
