/*
  BearSSLHelpers.h - esp8266 BearSSL support classes (ESP8266-compatible).

  Compile-compatible shim for the Ai-WB2-12F (BL602). The real BearSSL stack
  is not linked, so these classes parse nothing: the constructors are no-ops
  and the PEM/DER material is discarded. That keeps every BearSSL example
  (HTTPSRequest, BearSSL_Server, SecureBearSSLUpdater, ...) compiling unchanged
  while `WiFiClientSecure` actually connects over plain TCP (see its header).
 */
#ifndef BEARSSLHELPERS_H_
#define BEARSSLHELPERS_H_

#include <stddef.h>
#include <stdint.h>

/* BearSSL cipher-suite IDs / key-type flags used by the ESP8266 BearSSL
 * examples (setCiphersLessSecure(), setECCert(), ...). Values are the real
 * BearSSL constants. */
#define BR_TLS_RSA_WITH_AES_256_CBC_SHA256        0x003D
#define BR_TLS_RSA_WITH_AES_128_CBC_SHA           0x002F
#define BR_TLS_RSA_WITH_3DES_EDE_CBC_SHA          0x000A
#define BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA   0xC009
#define BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA   0xC00A
#define BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA     0xC013
#define BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA     0xC014
#define BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 0xC02B
#define BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384 0xC02C
#define BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256   0xC02F
#define BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384   0xC030

#define BR_KEYTYPE_KEYX 0x01
#define BR_KEYTYPE_SIGN 0x02

namespace BearSSL {

/* X509 certificate chain (PEM). parse() is a no-op on WB2. */
class X509List {
public:
    X509List() {}
    X509List(const char* pem) { (void)pem; }
    X509List(X509List&) {}
    X509List(const X509List&) {}
    X509List& operator=(const X509List&) { return *this; }
    ~X509List() {}
    void append(const char* pem) { (void)pem; }
    bool parse() { return true; }
    int getCount() { return 0; }
};

/* Private key (PEM or DER). parse() is a no-op on WB2. */
class PrivateKey {
public:
    PrivateKey() {}
    PrivateKey(const char* pem) { (void)pem; }
    PrivateKey(const uint8_t* der, size_t der_len) { (void)der; (void)der_len; }
    bool parse() { return true; }
};

/* Public key (PEM or DER). parse() is a no-op on WB2. */
class PublicKey {
public:
    PublicKey() {}
    PublicKey(const char* pem) { (void)pem; }
    PublicKey(const uint8_t* der, size_t der_len) { (void)der; (void)der_len; }
    bool parse() { return true; }
};

/* TLS session (session-resumption container). No state on WB2. */
class Session {
public:
    Session() {}
    int getParameters() const { return 0; }
};

/* Server-side session slot. */
class ServerSession {
public:
    ServerSession() {}
};

/* Fixed-size cache of server-side sessions. */
class ServerSessions {
public:
    ServerSessions(int maxSessions) { (void)maxSessions; }
    ServerSessions(ServerSession* store, int maxSessions) { (void)store; (void)maxSessions; }
    bool load(const Session& session) { (void)session; return true; }
    bool save(Session& session) { (void)session; return true; }
};

} // namespace BearSSL

/* Global-scope abstract base for certificate stores; BearSSL::CertStore
 * derives from it in the reference. Kept virtual so the vtable exists. */
class CertificateStore {
public:
    virtual ~CertificateStore() {}
};

#endif /* BEARSSLHELPERS_H_ */
