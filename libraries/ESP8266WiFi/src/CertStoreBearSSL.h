/*
  CertStoreBearSSL.h - esp8266 BearSSL certificate store (ESP8266-compatible).

  Compile-compatible shim for the Ai-WB2-12F (BL602). The real implementation
  loads a cert index+data pair from LittleFS into a BearSSL store; on WB2 the
  TLS stack is a plain-TCP shim, so initCertStore() reports zero certs (the
  caller treats that as "nothing to validate") and setCertStore() is a no-op.
 */
#ifndef CERTSTOREBEARSSL_H_
#define CERTSTOREBEARSSL_H_

#include "BearSSLHelpers.h"
#include <FS.h>
#include <pgmspace.h>

namespace BearSSL {

class CertStore : public CertificateStore {
public:
    CertStore() {}

    /* Load an index+data cert pair from an FS. No-op: returns 0 certs. */
    int initCertStore(fs::FS& fs, const char* indexFile, const char* dataFile) {
        (void)fs; (void)indexFile; (void)dataFile;
        return 0;
    }
    int initCertStore(fs::FS& fs, String indexFile, String dataFile) {
        (void)fs; (void)indexFile; (void)dataFile;
        return 0;
    }

    /* X.509 "notBefore/notAfter" validity is unchecked on WB2; keep the
     * validator hook so BearSSL_Validation-style code compiles. */
    void setDateTimeValidator(bool (*cb)(const char* str)) { (void)cb; }

    int getCount() { return 0; }
};

} // namespace BearSSL

#endif /* CERTSTOREBEARSSL_H_ */
