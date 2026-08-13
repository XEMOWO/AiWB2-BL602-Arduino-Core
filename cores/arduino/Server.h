/*
 * Server.h — abstract server base class (ESP8266-compatible).
 *
 * The base class is minimal (just begin()); the concrete WiFiServer in the
 * ESP8266 core adds available()/accept() itself. Third-party code only needs
 * Server* as a Print-able abstraction, so this matches the reference exactly.
 *
 * Mirror of cores/esp8266/Server.h in the esp8266/Arduino repo.
 */
#ifndef server_h
#define server_h

#include "Print.h"

class Server : public Print
{
public:
    virtual void begin() = 0;
};

#endif /* server_h */
