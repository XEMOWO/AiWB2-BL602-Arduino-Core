/*
 * Client.h — abstract client base class (ESP8266-compatible).
 *
 * Every concrete network client (WiFiClient, EthernetClient-like) derives
 * from this. Third-party code usually holds a Client* and calls the virtuals,
 * so the exact ESP8266 signature set matters: connect() returning int (1 on
 * success), read(uint8_t*,size_t), connected(), operator bool().
 *
 * Mirror of cores/esp8266/Client.h in the esp8266/Arduino repo.
 */
#ifndef client_h
#define client_h

#include "Print.h"
#include "Stream.h"
#include "IPAddress.h"

class Client : public Stream
{
public:
    virtual int connect(IPAddress ip, uint16_t port) = 0;
    virtual int connect(const char *host, uint16_t port) = 0;
    virtual size_t write(uint8_t) override = 0;
    virtual size_t write(const uint8_t *buf, size_t size) override = 0;
    virtual int available() override = 0;
    virtual int read() override = 0;
    virtual int read(uint8_t *buf, size_t size) override = 0;
    virtual int peek() override = 0;
    virtual void flush() override = 0;
    virtual void stop() = 0;
    virtual uint8_t connected() = 0;
    virtual operator bool() = 0;

protected:
    uint8_t *rawIPAddress(IPAddress &addr)       { return addr.raw_address(); }
    const uint8_t *rawIPAddress(const IPAddress &addr) { return addr.raw_address(); }
};

#endif /* client_h */
