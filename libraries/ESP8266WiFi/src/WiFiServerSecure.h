/*
  WiFiServerSecure.h - esp8266 TLS server (ESP8266-compatible).

  Compile-compatible shim for the Ai-WB2-12F (BL602). TLS configuration methods
  are no-ops; the server actually listens over plain TCP (WiFiServer). The
  accept()/available() overrides return WiFiClientSecure so BearSSL_Server-style
  sketches (`BearSSL::WiFiClientSecure incoming = server.accept();`) compile.
  `BearSSL::WiFiServerSecure` is an alias for this class.
 */
#ifndef WIFISERVERSECURE_H_
#define WIFISERVERSECURE_H_

#include "WiFiServer.h"
#include "WiFiClientSecure.h"
#include "BearSSLHelpers.h"

class WiFiServerSecure : public WiFiServer {
public:
    WiFiServerSecure(IPAddress addr, uint16_t port) : WiFiServer(addr, port) {}
    WiFiServerSecure(uint16_t port = 443) : WiFiServer(port) {}

    /* ---- TLS configuration. All no-ops (see WiFiClientSecure.h). ---- */
    void setRSACert(BearSSL::X509List* chain, BearSSL::PrivateKey* key) { (void)chain; (void)key; }
    void setRSACert(BearSSL::X509List& chain, BearSSL::PrivateKey& key) { (void)chain; (void)key; }
    void setECCert(BearSSL::X509List* chain, uint32_t keyType, BearSSL::PrivateKey* key) { (void)chain; (void)keyType; (void)key; }
    void setECDSACert(BearSSL::X509List* chain, BearSSL::PrivateKey* key) { (void)chain; (void)key; }
    void setClientTrustAnchor(BearSSL::X509List* anchor) { (void)anchor; }
    void setClientTrustAnchor(BearSSL::X509List& anchor) { (void)anchor; }
    void setCache(BearSSL::ServerSessions* cache) { (void)cache; }
    void setBufferSizes(int recv, int xmit) { (void)recv; (void)xmit; }

    /* Claim an incoming connection as a TLS client (hides the base
     * WiFiClient-returning versions; these are non-virtual, so hiding is
     * fine). The base's lwIP socket is handed to the WiFiClientSecure ctor. */
    WiFiClientSecure accept() { return WiFiClientSecure(WiFiServer::accept()); }
    WiFiClientSecure available(uint8_t* status = NULL) { (void)status; return accept(); }
};

/* BearSSL namespace alias so `BearSSL::WiFiServerSecure server(443);` parses. */
namespace BearSSL {
using WiFiServerSecure = ::WiFiServerSecure;
}

#endif /* WIFISERVERSECURE_H_ */
