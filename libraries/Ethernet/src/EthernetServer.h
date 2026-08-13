/*
 * EthernetServer.h - classic Arduino Ethernet server for the Ai-WB2-12F (BL602).
 *
 * Compile-compatible shim over WiFiServer (see Ethernet.h). available()/accept()
 * return one pending client like the classic API, and write() broadcasts to the
 * clients handed out so far (matching the classic "echo to every client"
 * semantics the chat/echo examples rely on).
 */

#ifndef __ETHERNET_SERVER_H__
#define __ETHERNET_SERVER_H__

#include <Arduino.h>
#include <Server.h>
#include <WiFiServer.h>

#include "EthernetClient.h"

class EthernetServer : public Server
{
public:
    EthernetServer(uint16_t port = 23)
        : _server(port), _port(port) {}

    void begin() override { _server.begin(); }
    void begin(uint16_t port) { _port = port; _server.begin(port); }

    // classic EthernetServer::available() / newer accept(): one pending client
    EthernetClient available()
    {
        WiFiClient c = _server.accept();
        if (c) { _track(c); }
        return c;
    }
    EthernetClient accept()
    {
        WiFiClient c = _server.accept();
        if (c) { _track(c); }
        return c;
    }

    // broadcast to the clients this server has handed out (classic semantics)
    size_t write(uint8_t b)
    {
        size_t n = 0;
        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (_clients[i]) { n += _clients[i].write(b); }
        }
        return n;
    }
    size_t write(const uint8_t *buf, size_t size)
    {
        size_t n = 0;
        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (_clients[i]) { n += _clients[i].write(buf, size); }
        }
        return n;
    }
    using Print::write;

    EthernetClient operator[](int idx)
    {
        return (idx >= 0 && idx < MAX_SOCKETS) ? _clients[idx] : WiFiClient();
    }

    bool hasClient() { return _server.hasClient(); }
    void setNoDelay(bool nodelay) { _server.setNoDelay(nodelay); }
    bool getNoDelay() { return _server.getNoDelay(); }
    uint16_t port() const { return _port; }
    uint8_t status() { return _server.status(); }

    void stop() { _server.stop(); }
    void end() { _server.end(); }
    void close() { _server.stop(); }
    void stopAll()
    {
        _server.stop();
        for (int i = 0; i < MAX_SOCKETS; i++) { _clients[i].stop(); }
    }

    explicit operator bool() { return (bool)_server; }

private:
    void _track(WiFiClient &c)
    {
        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (!_clients[i]) { _clients[i] = c; return; }
        }
        // list full: drop the oldest so we keep accepting
        _clients[0] = c;
    }

    static const int MAX_SOCKETS = 8;
    WiFiServer _server;
    WiFiClient _clients[MAX_SOCKETS];
    uint16_t _port;
};

#endif /* __ETHERNET_SERVER_H__ */
