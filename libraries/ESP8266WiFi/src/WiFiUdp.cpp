/*
  WiFiUdp.cpp - esp8266 WiFi UDP class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

  Ported to Ai-WB2-12F (BL602): backend is the SDK's lwIP socket API. The
  outgoing datagram is built up by write() calls and sent atomically by
  endPacket(); parsePacket() pulls one datagram into the rx buffer. Copies of a
  WiFiUDP share one UdpContext (refcounted); the last copy to die closes the
  socket.
 */
#include "WB2WiFiCommon.h"
#include "lwip_socket_util.h"
#include "WiFiUdp.h"

#include <list>

struct WiFiUDP::UdpContext {
    int sock = -1;
    uint32_t refs = 1;

    uint8_t* txBuf = nullptr;   /* datagram being built by write() */
    size_t txLen = 0;
    size_t txCap = 0;
    IPAddress txIP;
    uint16_t txPort = 0;

    uint8_t* rxBuf = nullptr;   /* current received datagram */
    size_t rxLen = 0;
    size_t rxIdx = 0;
    size_t rxCap = 0;

    IPAddress remoteIP;
    uint16_t remotePort = 0;
    IPAddress multicast;   /* group address, for destinationIP() */
};

static std::list<WiFiUDP::UdpContext*> s_live;

static void s_ctx_register(WiFiUDP::UdpContext* c) { s_live.push_back(c); }
static void s_ctx_unregister(WiFiUDP::UdpContext* c) { s_live.remove(c); }

static void s_ctx_free(WiFiUDP::UdpContext* c)
{
    if (!c) return;
    if (c->sock >= 0) lwip_close(c->sock);
    if (c->txBuf) free(c->txBuf);
    if (c->rxBuf) free(c->rxBuf);
    delete c;
}

static void s_ctx_unref(WiFiUDP::UdpContext* c)
{
    if (!c) return;
    if (--c->refs == 0) {
        s_ctx_unregister(c);
        s_ctx_free(c);
    }
}

/* ---- construction / assignment ---- */

WiFiUDP::WiFiUDP() : _ctx(nullptr) {}

WiFiUDP::WiFiUDP(const WiFiUDP& other) : _ctx(other._ctx)
{
    if (_ctx) _ctx->refs++;
}

WiFiUDP& WiFiUDP::operator=(const WiFiUDP& rhs)
{
    if (this == &rhs) return *this;
    s_ctx_unref(_ctx);
    _ctx = rhs._ctx;
    if (_ctx) _ctx->refs++;
    return *this;
}

WiFiUDP::~WiFiUDP()
{
    s_ctx_unref(_ctx);
    _ctx = nullptr;
}

/* ---- begin / stop ---- */

uint8_t WiFiUDP::begin(uint16_t port)
{
    if (!_ctx) {
        _ctx = new UdpContext();
        s_ctx_register(_ctx);
    } else {
        if (_ctx->sock >= 0) lwip_close(_ctx->sock);
        _ctx->sock = -1;
        _ctx->rxLen = _ctx->rxIdx = _ctx->txLen = 0;
        _ctx->multicast = IPAddress();
    }

    int fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return 0;

    struct sockaddr_in addr = s_wb2_sockaddr(IPAddress(0, 0, 0, 0), port);
    if (lwip_bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        lwip_close(fd);
        return 0;
    }
    s_wb2_set_nonblock(fd);
    _ctx->sock = fd;
    return 1;
}

void WiFiUDP::stop()
{
    if (!_ctx) return;
    if (_ctx->sock >= 0) {
        lwip_close(_ctx->sock);
        _ctx->sock = -1;
    }
    _ctx->rxLen = _ctx->rxIdx = _ctx->txLen = 0;
}

uint8_t WiFiUDP::beginMulticast(IPAddress multicast, uint16_t port)
{
    return beginMulticast(IPAddress(0, 0, 0, 0), multicast, port);
}

uint8_t WiFiUDP::beginMulticast(IPAddress interfaceAddr, IPAddress multicast, uint16_t port)
{
    if (!begin(port)) return 0;

    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = multicast.asUint32();
    mreq.imr_interface.s_addr = interfaceAddr.asUint32();
    if (lwip_setsockopt(_ctx->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0) {
        /* IGMP is enabled in this SDK; a failure still leaves the socket
         * bound on the port. */
    }
    _ctx->multicast = multicast;
    return 1;
}

/* ---- sending ---- */

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port)
{
    if (!_ctx) return 0;
    if (ip.asUint32() == 0) return 0;
    _ctx->txIP = ip;
    _ctx->txPort = port;
    _ctx->txLen = 0;
    return 1;
}

int WiFiUDP::beginPacket(const char* host, uint16_t port)
{
    IPAddress ip;
    if (!s_wb2_resolve(host, &ip)) return 0;
    return beginPacket(ip, port);
}

int WiFiUDP::beginPacketMulticast(IPAddress multicastAddress, uint16_t port,
                                  IPAddress interfaceAddress, int ttl)
{
    if (_ctx && _ctx->sock >= 0) {
#ifdef IP_MULTICAST_TTL
        lwip_setsockopt(_ctx->sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
#endif
#ifdef IP_MULTICAST_IF
        uint32_t iface = interfaceAddress.asUint32();
        lwip_setsockopt(_ctx->sock, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface));
#endif
    }
    return beginPacket(multicastAddress, port);
}

int WiFiUDP::endPacket()
{
    if (!_ctx || _ctx->sock < 0) {
        if (_ctx) _ctx->txLen = 0;
        return 0;
    }

    struct sockaddr_in addr = s_wb2_sockaddr(_ctx->txIP, _ctx->txPort);
    int rc = lwip_sendto(_ctx->sock, _ctx->txBuf, _ctx->txLen, 0,
                         (struct sockaddr*)&addr, sizeof(addr));
    _ctx->txLen = 0;
    return (rc >= 0) ? 1 : 0;
}

size_t WiFiUDP::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiUDP::write(const uint8_t* buffer, size_t size)
{
    if (!_ctx) return 0;

    if (_ctx->txLen + size > UDP_TX_PACKET_MAX_SIZE) {
        size = UDP_TX_PACKET_MAX_SIZE - _ctx->txLen;
    }
    if (size == 0) return 0;

    if (_ctx->txLen + size > _ctx->txCap) {
        size_t newCap = _ctx->txCap ? _ctx->txCap : 256;
        while (newCap < _ctx->txLen + size) newCap *= 2;
        if (newCap > UDP_TX_PACKET_MAX_SIZE) newCap = UDP_TX_PACKET_MAX_SIZE;
        uint8_t* nb = (uint8_t*)realloc(_ctx->txBuf, newCap);
        if (!nb) return 0;
        _ctx->txBuf = nb;
        _ctx->txCap = newCap;
    }

    memcpy(_ctx->txBuf + _ctx->txLen, buffer, size);
    _ctx->txLen += size;
    return size;
}

/* ---- receiving ---- */

int WiFiUDP::parsePacket()
{
    if (!_ctx) return 0;
    _ctx->rxLen = _ctx->rxIdx = 0;
    if (_ctx->sock < 0) return 0;

    /* FIONREAD on a datagram socket reports the next datagram's size. */
    int pending = 0;
    if (lwip_ioctl(_ctx->sock, FIONREAD, &pending) < 0 || pending <= 0) return 0;

    if ((size_t)pending > _ctx->rxCap) {
        uint8_t* nb = (uint8_t*)realloc(_ctx->rxBuf, (size_t)pending);
        if (!nb) return 0;
        _ctx->rxBuf = nb;
        _ctx->rxCap = (size_t)pending;
    }

    struct sockaddr_in from;
    socklen_t flen = sizeof(from);
    int got = lwip_recvfrom(_ctx->sock, _ctx->rxBuf, (size_t)pending, 0,
                            (struct sockaddr*)&from, &flen);
    if (got < 0) return 0;

    _ctx->rxLen = (size_t)got;
    _ctx->rxIdx = 0;
    _ctx->remoteIP = IPAddress(from.sin_addr.s_addr);
    _ctx->remotePort = ntohs(from.sin_port);
    return got;
}

int WiFiUDP::available()
{
    return _ctx ? (int)(_ctx->rxLen - _ctx->rxIdx) : 0;
}

int WiFiUDP::read()
{
    if (!_ctx || _ctx->rxIdx >= _ctx->rxLen) return -1;
    return _ctx->rxBuf[_ctx->rxIdx++];
}

int WiFiUDP::read(unsigned char* buffer, size_t len)
{
    if (!_ctx) return 0;
    size_t n = _ctx->rxLen - _ctx->rxIdx;
    if (n > len) n = len;
    if (n == 0) return 0;
    memcpy(buffer, _ctx->rxBuf + _ctx->rxIdx, n);
    _ctx->rxIdx += n;
    return (int)n;
}

int WiFiUDP::peek()
{
    if (!_ctx || _ctx->rxIdx >= _ctx->rxLen) return -1;
    return _ctx->rxBuf[_ctx->rxIdx];
}

void WiFiUDP::flush()
{
    if (_ctx) _ctx->rxLen = _ctx->rxIdx = 0;
}

/* ---- metadata ---- */

IPAddress WiFiUDP::remoteIP() { return _ctx ? _ctx->remoteIP : IPAddress(); }
uint16_t WiFiUDP::remotePort() { return _ctx ? _ctx->remotePort : 0; }
IPAddress WiFiUDP::destinationIP() const { return _ctx ? _ctx->multicast : IPAddress(); }

uint16_t WiFiUDP::localPort() const
{
    if (!_ctx || _ctx->sock < 0) return 0;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (lwip_getsockname(_ctx->sock, (struct sockaddr*)&addr, &len) != 0) return 0;
    return ntohs(addr.sin_port);
}

/* ---- stopAll ---- */

void WiFiUDP::stopAll()
{
    for (std::list<UdpContext*>::iterator it = s_live.begin(); it != s_live.end(); ++it) {
        UdpContext* c = *it;
        if (c->sock >= 0) {
            lwip_close(c->sock);
            c->sock = -1;
        }
        c->rxLen = c->rxIdx = c->txLen = 0;
    }
}

void WiFiUDP::stopAllExcept(WiFiUDP* exC)
{
    UdpContext* keep = exC ? exC->_ctx : nullptr;
    for (std::list<UdpContext*>::iterator it = s_live.begin(); it != s_live.end(); ++it) {
        UdpContext* c = *it;
        if (c == keep) continue;
        if (c->sock >= 0) {
            lwip_close(c->sock);
            c->sock = -1;
        }
        c->rxLen = c->rxIdx = c->txLen = 0;
    }
}
