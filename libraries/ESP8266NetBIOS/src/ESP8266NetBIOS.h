/*
 * ESP8266NetBIOS.h - ESP8266-compatible NetBIOS name responder for the WB2.
 *
 * Compile-compatible port. A real NBNS responder answers NetBIOS name-query
 * broadcasts (UDP 137); BL602's lwIP has no NBNS module, so begin() reports
 * success but answers nothing.
 */

#ifndef ESP8266NETBIOS_H
#define ESP8266NETBIOS_H

#include <Arduino.h>

class NBNSClass
{
public:
    NBNSClass() {}

    /* Start answering NetBIOS name queries for `name` (max 15 chars). */
    bool begin(const char *name)
    {
        (void)name;
        return true;
    }

    void end() {}
};

extern NBNSClass NBNS;

#endif // ESP8266NETBIOS_H
