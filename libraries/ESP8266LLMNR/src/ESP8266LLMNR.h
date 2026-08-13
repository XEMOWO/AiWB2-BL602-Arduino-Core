/*
 * ESP8266LLMNR.h - ESP8266-compatible LLMNR responder for the Ai-WB2-12F.
 *
 * Compile-compatible port. A real LLMNR (RFC 4795) responder would listen for
 * multicast name queries on UDP 5355; the BL602 lwIP has no LLMNR module, so
 * begin() reports success but serves nothing — sketches compile and run, and
 * the hostname resolves only via mDNS/DNS if those are running.
 */

#ifndef ESP8266LLMNR_H
#define ESP8266LLMNR_H

#include <Arduino.h>

class LLMNRClass
{
public:
    LLMNRClass() {}

    /* Start the responder with the given hostname (no dots). */
    bool begin(const char *name)
    {
        (void)name;
        return true;
    }

    void end() {}
};

extern LLMNRClass LLMNR;

#endif // ESP8266LLMNR_H
