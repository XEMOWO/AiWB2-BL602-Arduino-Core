/*
  WiFiServer.cpp - esp8266 WiFi server class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

  Ported to Ai-WB2-12F (BL602): backend is the SDK's lwIP socket API. A single
  non-blocking listening socket backs the server; pending connections sit in
  the kernel backlog (default 5, like the reference MAX_PENDING_CLIENTS_PER_PORT)
  until accept()/available() claims them. Accepted sockets are handed to
  WiFiClient, which owns them.
 */
#include "WB2WiFiCommon.h"
#include "lwip_socket_util.h"
#include "WiFiServer.h"

#define WB2_DEFAULT_BACKLOG 5
#define WB2_TCP_STATE_CLOSED        0   /* lwIP tcp_state */
#define WB2_TCP_STATE_ESTABLISHED   4

WiFiServer::WiFiServer(const IPAddress& addr, uint16_t port)
    : _port(port), _addr(addr), _listen_fd(-1), _noDelay(_ndDefault) {}

WiFiServer::WiFiServer(uint16_t port)
    : _port(port), _listen_fd(-1), _noDelay(_ndDefault) {}

WiFiServer::~WiFiServer()
{
    close();
}

void WiFiServer::begin()
{
    begin(_port);
}

void WiFiServer::begin(uint16_t port)
{
    begin(port, WB2_DEFAULT_BACKLOG);
}

void WiFiServer::begin(uint16_t port, uint8_t backlog)
{
    _port = port;
    close();

    int fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return;

    int one = 1;
    lwip_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = _addr.asUint32();   /* INADDR_ANY == 0 */

    if (lwip_bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        lwip_close(fd);
        return;
    }
    if (lwip_listen(fd, backlog) != 0) {
        lwip_close(fd);
        return;
    }
    s_wb2_set_nonblock(fd);
    _listen_fd = fd;
}

WiFiClient WiFiServer::accept()
{
    if (_listen_fd < 0) return WiFiClient();

    struct sockaddr_in from;
    socklen_t len = sizeof(from);
    int fd = lwip_accept(_listen_fd, (struct sockaddr*)&from, &len);
    if (fd < 0) return WiFiClient();   /* EAGAIN = nothing pending */

    WiFiClient client(fd);
    switch (_noDelay) {
    case _ndTrue:  client.setNoDelay(true); break;
    case _ndFalse: client.setNoDelay(false); break;
    default:       client.setNoDelay(WiFiClient::getDefaultNoDelay()); break;
    }
    return client;
}

WiFiClient WiFiServer::available(uint8_t* status)
{
    WiFiClient c = accept();
    if (status) {
        *status = c ? WB2_TCP_STATE_ESTABLISHED : WB2_TCP_STATE_CLOSED;
    }
    return c;
}

bool WiFiServer::hasClient()
{
    if (_listen_fd < 0) return false;

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(_listen_fd, &rset);
    struct timeval tv = { 0, 0 };
    int rc = lwip_select(_listen_fd + 1, &rset, NULL, NULL, &tv);
    return rc > 0 && FD_ISSET(_listen_fd, &rset);
}

size_t WiFiServer::hasClientData()
{
    /* A pending connection's payload can't be probed without accepting it;
     * report "a pending client exists", which is what callers gate on. */
    return hasClient() ? 1 : 0;
}

bool WiFiServer::hasMaxPendingClients()
{
    return false;   /* lwIP enforces the backlog internally */
}

void WiFiServer::setNoDelay(bool nodelay)
{
    _noDelay = nodelay ? _ndTrue : _ndFalse;
}

bool WiFiServer::getNoDelay()
{
    if (_noDelay == _ndDefault) return WiFiClient::getDefaultNoDelay();
    return _noDelay == _ndTrue;
}

uint8_t WiFiServer::status()
{
    return _listen_fd >= 0 ? WB2_TCP_STATE_ESTABLISHED : WB2_TCP_STATE_CLOSED;
}

uint16_t WiFiServer::port() const
{
    return _port;
}

void WiFiServer::close()
{
    if (_listen_fd >= 0) {
        lwip_close(_listen_fd);
        _listen_fd = -1;
    }
}

void WiFiServer::stop()
{
    close();
}

void WiFiServer::end()
{
    close();
}

WiFiServer::operator bool()
{
    return _listen_fd >= 0;
}
