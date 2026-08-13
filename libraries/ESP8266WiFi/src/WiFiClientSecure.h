/*
  WiFiClientSecure.h - esp8266 TLS client (ESP8266-compatible).

  Compile-compatible shim for the Ai-WB2-12F (BL602). The real BearSSL stack
  is not linked, so every TLS configuration method is a no-op and the actual
  connection runs over plain TCP (inherited from WiFiClient). HTTPS/BearSSL
  sketches therefore compile unchanged; the payload transfer works, minus the
  encryption/validation. `BearSSL::WiFiClientSecure` is an alias for this class
  so `#include <WiFiClientSecureBearSSL.h>` sketches compile too.
 */
#ifndef WIFICLIENTSECURE_H_
#define WIFICLIENTSECURE_H_

#include "WiFiClient.h"
#include "BearSSLHelpers.h"
#include "CertStoreBearSSL.h"

#include <vector>

class WiFiClientSecure : public WiFiClient {
public:
    WiFiClientSecure() : WiFiClient() {}
    WiFiClientSecure(int fd) : WiFiClient(fd) {}
    WiFiClientSecure(const WiFiClient& c) : WiFiClient(c) {}

    /* ---- TLS configuration. All no-ops: WB2 has no BearSSL backend, so the
     * connection underneath is the plain-lwIP WiFiClient. ---- */
    void setInsecure() {}
    void allowSelfSignedCerts() {}
    void setTrustAnchors(BearSSL::X509List* trustAnchors) { (void)trustAnchors; }
    void setTrustAnchors(const BearSSL::X509List* trustAnchors) { (void)trustAnchors; }
    void setKnownKey(BearSSL::PublicKey* knownKey) { (void)knownKey; }
    void setKnownKey(const BearSSL::PublicKey* knownKey) { (void)knownKey; }
    void setClientCert(BearSSL::X509List* clientCert) { (void)clientCert; }
    void setClientCert(const BearSSL::X509List* clientCert) { (void)clientCert; }
    void setPrivateKey(BearSSL::PrivateKey* privateKey) { (void)privateKey; }
    void setPrivateKey(const BearSSL::PrivateKey* privateKey) { (void)privateKey; }
    void setFingerprint(const uint8_t fp[20]) { (void)fp; }
    void setFingerprint(const char* fp) { (void)fp; }
    void setCACert(const char* rootCert) { (void)rootCert; }
    void setCACert(const BearSSL::X509List* rootCert) { (void)rootCert; }
    void setCertStore(BearSSL::CertStore* certStore) { (void)certStore; }
    void setBufferSizes(int recv, int xmit) { (void)recv; (void)xmit; }
    void setSession(BearSSL::Session* session) { (void)session; }
    void setSession(BearSSL::Session& session) { (void)session; }
    void setCiphersLessSecure() {}
    void setCiphers(const std::vector<uint16_t>& ciphers) { (void)ciphers; }

    /* Probe the peer's max-fragment-length extension. No TLS here, so there is
     * nothing to probe: report false (the caller then skips setBufferSizes). */
    bool probeMaxFragmentLength(const char* host, uint16_t port, uint32_t timeout) {
        (void)host; (void)port; (void)timeout;
        return false;
    }
    /* MFLN was never negotiated (probeMaxFragmentLength returned false). */
    bool getMFLNStatus() const { return false; }
};

/* BearSSL namespace aliases so `BearSSL::WiFiClientSecure client;` parses. */
namespace BearSSL {
using WiFiClientSecure = ::WiFiClientSecure;
}

/* The reference does exactly this: it opens the BearSSL namespace at global
 * scope so unqualified `ESP8266WebServerSecure`, `ESP8266HTTPUpdateServerSecure`
 * and `X509List` resolve to the BearSSL aliases. Same entities as the global
 * classes, so no ambiguity. */
using namespace BearSSL;

#endif /* WIFICLIENTSECURE_H_ */
