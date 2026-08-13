/*
  WiFiClient.cpp - esp8266 WiFi client class (ESP8266-compatible).
  Copyright (c) 2011-2014 Arduino.  All right reserved.

  Ported to Ai-WB2-12F (BL602): backend is the SDK's lwIP socket API.

  Semantics (matching the reference ClientContext where it matters):
   - reads are non-blocking: available()/read()/peek() never stall;
   - write() pushes data with a bounded wait (WIFICLIENT_MAX_FLUSH_WAIT_MS) and
     returns what it managed to send;
   - connect() is non-blocking with a 5 s select() timeout, returns 1/0 like
     the reference;
   - copies share one WiFiClientContext (refcounted); the last copy to die
     closes the connection;
   - WiFiClient::stopAll()/stopAllExcept() close every live socket.
 */
#include "WB2WiFiCommon.h"
#include "lwip_socket_util.h"
#include "WiFiClient.h"

#include <list>

#define WB2_TCP_CONNECT_TIMEOUT_MS 5000

/* lwIP tcp_state values used by status() (see lwip/tcp.h). */
#define WB2_TCP_STATE_CLOSED        0
#define WB2_TCP_STATE_ESTABLISHED   4

/* ---- internal shared state ---- */

struct WiFiClient::WiFiClientContext {
    int sock = -1;
    bool connected = false;
    uint32_t refs = 1;

    IPAddress remoteIP, localIP;
    uint16_t remotePort = 0, localPort = 0;

    uint8_t peekByte = 0;   /* 1-byte peek buffer (peekBuffer API) */
    bool peeked = false;

    bool noDelay = false;
    bool sync = false;
    uint16_t keepAliveIdleSec = 0;
    uint16_t keepAliveIntervalSec = 0;
    uint8_t keepAliveCount = 0;
};

static std::list<WiFiClient::WiFiClientContext*> s_live;

static void s_ctx_register(WiFiClient::WiFiClientContext* c) { s_live.push_back(c); }
static void s_ctx_unregister(WiFiClient::WiFiClientContext* c) { s_live.remove(c); }

static void s_ctx_close(WiFiClient::WiFiClientContext* c)
{
    if (c && c->sock >= 0) {
        lwip_close(c->sock);
        c->sock = -1;
        c->connected = false;
        c->peeked = false;
    }
}

static void s_ctx_unref(WiFiClient::WiFiClientContext* c)
{
    if (!c) return;
    if (--c->refs == 0) {
        s_ctx_unregister(c);
        s_ctx_close(c);
        delete c;
    }
}

/* ---- static state ---- */

uint16_t WiFiClient::_localPort = 0;

static bool s_defaultNoDelay = false;
static bool s_defaultSync = false;

/* ---- option helpers ---- */

static void s_apply_keepalive(WiFiClient::WiFiClientContext* c)
{
    if (!c || c->sock < 0) return;
    int enable = (c->keepAliveIdleSec != 0) ? 1 : 0;
    lwip_setsockopt(c->sock, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
    if (enable) {
        int v = c->keepAliveIdleSec;
        lwip_setsockopt(c->sock, IPPROTO_TCP, TCP_KEEPIDLE, &v, sizeof(v));
        v = c->keepAliveIntervalSec;
        lwip_setsockopt(c->sock, IPPROTO_TCP, TCP_KEEPINTVL, &v, sizeof(v));
        v = c->keepAliveCount;
        lwip_setsockopt(c->sock, IPPROTO_TCP, TCP_KEEPCNT, &v, sizeof(v));
    }
}

static void s_apply_nodelay(WiFiClient::WiFiClientContext* c)
{
    if (!c || c->sock < 0) return;
    int v = c->noDelay ? 1 : 0;
    lwip_setsockopt(c->sock, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
}

static void s_cache_addrs(WiFiClient::WiFiClientContext* c)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (lwip_getpeername(c->sock, (struct sockaddr*)&addr, &len) == 0) {
        c->remoteIP = IPAddress(addr.sin_addr.s_addr);
        c->remotePort = ntohs(addr.sin_port);
    }
    len = sizeof(addr);
    if (lwip_getsockname(c->sock, (struct sockaddr*)&addr, &len) == 0) {
        c->localIP = IPAddress(addr.sin_addr.s_addr);
        c->localPort = ntohs(addr.sin_port);
    }
}

/* Non-blocking connect with a select() timeout. Returns 0 on success. */
static int s_connect_timeout(int fd, const struct sockaddr* addr, socklen_t len,
                             uint32_t timeoutMs, int* err_out)
{
    int rc = lwip_connect(fd, addr, len);
    if (rc == 0) return 0;
    int e = errno;
    if (e != EINPROGRESS && e != EWOULDBLOCK && e != EAGAIN) {
        *err_out = e;
        return -1;
    }
    fd_set wset, eset;
    FD_ZERO(&wset); FD_SET(fd, &wset);
    FD_ZERO(&eset); FD_SET(fd, &eset);
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    rc = lwip_select(fd + 1, NULL, &wset, &eset, &tv);
    if (rc <= 0) {
        *err_out = (rc == 0) ? ETIMEDOUT : errno;
        return -1;
    }
    int soerr = 0;
    socklen_t olen = sizeof(soerr);
    if (lwip_getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &olen) == 0 && soerr != 0) {
        *err_out = soerr;
        return -1;
    }
    return 0;
}

/* ---- construction / assignment ---- */

WiFiClient::WiFiClient() : _client(nullptr) {}

WiFiClient::WiFiClient(int sock)
{
    _client = new WiFiClientContext();
    _client->sock = sock;
    _client->connected = true;
    _client->noDelay = s_defaultNoDelay;
    _client->sync = s_defaultSync;
    s_ctx_register(_client);
    s_wb2_set_nonblock(sock);
    s_cache_addrs(_client);
}

WiFiClient::~WiFiClient()
{
    s_ctx_unref(_client);
    _client = nullptr;
}

WiFiClient::WiFiClient(const WiFiClient& other) : _client(other._client)
{
    if (_client) _client->refs++;
}

WiFiClient& WiFiClient::operator=(const WiFiClient& rhs)
{
    if (this == &rhs) return *this;
    s_ctx_unref(_client);
    _client = rhs._client;
    if (_client) _client->refs++;
    return *this;
}

std::unique_ptr<WiFiClient> WiFiClient::clone() const
{
    return std::unique_ptr<WiFiClient>(new WiFiClient(*this));
}

uint8_t WiFiClient::status()
{
    return connected() ? WB2_TCP_STATE_ESTABLISHED : WB2_TCP_STATE_CLOSED;
}

/* ---- connect ---- */

int WiFiClient::connect(IPAddress ip, uint16_t port)
{
    if (!_client) {
        _client = new WiFiClientContext();
        _client->noDelay = s_defaultNoDelay;
        _client->sync = s_defaultSync;
        s_ctx_register(_client);
    }
    s_ctx_close(_client);   /* drop any previous connection */

    if (ip.asUint32() == 0) {
        return 0;
    }

    int fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return 0;

    s_wb2_set_nonblock(fd);

    if (_localPort != 0) {
        struct sockaddr_in baddr = s_wb2_sockaddr(IPAddress(0, 0, 0, 0), _localPort++);
        if (lwip_bind(fd, (struct sockaddr*)&baddr, sizeof(baddr)) != 0) {
            lwip_close(fd);
            return 0;
        }
    }

    struct sockaddr_in addr = s_wb2_sockaddr(ip, port);
    int err = 0;
    if (s_connect_timeout(fd, (struct sockaddr*)&addr, sizeof(addr),
                          WB2_TCP_CONNECT_TIMEOUT_MS, &err) != 0) {
        lwip_close(fd);
        return 0;
    }

    _client->sock = fd;
    _client->connected = true;
    s_apply_nodelay(_client);
    s_apply_keepalive(_client);
    s_cache_addrs(_client);
    return 1;
}

int WiFiClient::connect(const char* host, uint16_t port)
{
    IPAddress ip;
    if (!s_wb2_resolve(host, &ip)) {
        return 0;
    }
    return connect(ip, port);
}

int WiFiClient::connect(const String& host, uint16_t port)
{
    return connect(host.c_str(), port);
}

/* ---- write ---- */

size_t WiFiClient::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiClient::write(const uint8_t* buf, size_t size)
{
    if (!_client || _client->sock < 0 || !size) return 0;

    size_t sent = 0;
    while (sent < size) {
        int rc = lwip_send(_client->sock, buf + sent, size - sent, 0);
        if (rc > 0) {
            sent += (size_t)rc;
            continue;
        }
        if (rc < 0) {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
                /* send buffer full: wait (bounded) for space to free up */
                if (s_wb2_wait_socket(_client->sock, true, WIFICLIENT_MAX_FLUSH_WAIT_MS) <= 0) {
                    break;
                }
                continue;
            }
            _client->connected = false;
            break;
        }
        /* rc == 0: peer closed */
        _client->connected = false;
        break;
    }

    if (_client->sync) {
        flush(WIFICLIENT_MAX_FLUSH_WAIT_MS);
    }
    return sent;
}

size_t WiFiClient::write_P(PGM_P buf, size_t size)
{
    /* flash is memory-mapped on BL602, so a normal write suffices */
    return write((const uint8_t*)buf, size);
}

size_t WiFiClient::write(Stream& stream)
{
    if (!_client || !stream.available()) return 0;

    uint8_t tmp[WIFICLIENT_MAX_PACKET_SIZE];
    size_t total = 0;
    int n;
    while ((n = stream.read(tmp, sizeof(tmp))) > 0) {
        size_t w = write(tmp, (size_t)n);
        total += w;
        if (w != (size_t)n) break;
    }
    return total;
}

/* ---- read ---- */

int WiFiClient::available()
{
    if (!_client || _client->sock < 0) return 0;
    int n = 0;
    if (lwip_ioctl(_client->sock, FIONREAD, &n) < 0 || n < 0) n = 0;
    return (int)((size_t)n + (_client->peeked ? 1 : 0));
}

int WiFiClient::read()
{
    uint8_t b;
    if (read(&b, 1) <= 0) return -1;
    return (int)b;
}

int WiFiClient::read(uint8_t* buf, size_t size)
{
    if (!_client || _client->sock < 0) return 0;
    if (size == 0) return 0;

    size_t got = 0;
    if (_client->peeked) {
        buf[0] = _client->peekByte;
        _client->peeked = false;
        got = 1;
        if (size == 1) return 1;
    }

    int rc = lwip_recv(_client->sock, buf + got, size - got, 0);
    if (rc > 0) return (int)(got + (size_t)rc);
    if (rc == 0) {
        _client->connected = false;   /* peer closed */
        return (int)got;
    }
    int e = errno;
    if (e == EAGAIN || e == EWOULDBLOCK) return (int)got;
    _client->connected = false;
    return (int)got;
}

int WiFiClient::read(char* buf, size_t size)
{
    return read((uint8_t*)buf, size);
}

int WiFiClient::peek()
{
    if (!_client || _client->sock < 0) return -1;
    if (_client->peeked) return _client->peekByte;

    uint8_t b;
    int rc = lwip_recv(_client->sock, &b, 1, MSG_PEEK);
    if (rc == 1) {
        _client->peeked = true;
        _client->peekByte = b;
        return (int)b;
    }
    if (rc == 0) _client->connected = false;
    return -1;
}

/* ---- peek-buffer API ---- */

bool WiFiClient::hasPeekBufferAPI() const { return true; }

size_t WiFiClient::peekAvailable()
{
    if (!_client || _client->sock < 0) return 0;
    if (!_client->peeked) {
        uint8_t b;
        if (lwip_recv(_client->sock, &b, 1, MSG_PEEK) == 1) {
            _client->peeked = true;
            _client->peekByte = b;
        } else {
            return 0;
        }
    }
    return 1;   /* 1-byte peek buffer (see header note) */
}

const char* WiFiClient::peekBuffer()
{
    if (peekAvailable() == 0) return nullptr;
    return (const char*)&_client->peekByte;
}

void WiFiClient::peekConsume(size_t consume)
{
    if (!_client) return;
    if (consume > 0 && _client->peeked) {
        _client->peeked = false;
    }
}

size_t WiFiClient::peekBytes(uint8_t* buffer, size_t length)
{
    if (!buffer || length == 0) return 0;
    size_t count = 0;
    while (count < length) {
        if (!peekAvailable()) break;
        buffer[count++] = _client->peekByte;
        _client->peeked = false;
    }
    return count;
}

/* ---- flush / stop / connected ---- */

bool WiFiClient::flush(unsigned int maxWaitMs)
{
    if (!_client || _client->sock < 0) return true;
    if (maxWaitMs == 0) return true;

    /* data is already in the kernel send path; wait (bounded) for it to
     * drain, i.e. for the socket to become writable again. */
    uint32_t start = millis();
    while ((unsigned long)(millis() - start) < maxWaitMs) {
        int rc = s_wb2_wait_socket(_client->sock, true, 10);
        if (rc > 0) return true;
        if (rc < 0) break;
    }
    return true;
}

bool WiFiClient::stop(unsigned int maxWaitMs)
{
    if (!_client) return true;
    if (_client->sock < 0) return true;

    if (maxWaitMs > 0) {
        /* graceful: stop sending, wait for the peer's FIN (up to maxWaitMs) */
        lwip_shutdown(_client->sock, SHUT_WR);
        uint32_t start = millis();
        while ((unsigned long)(millis() - start) < maxWaitMs) {
            fd_set rset;
            FD_ZERO(&rset);
            FD_SET(_client->sock, &rset);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 20000;
            int rc = lwip_select(_client->sock + 1, &rset, NULL, NULL, &tv);
            if (rc < 0) break;
            if (rc > 0) {
                uint8_t b;
                int rr = lwip_recv(_client->sock, &b, 1, 0);
                if (rr == 0) break;   /* peer FIN */
                if (rr < 0 && errno != EAGAIN && errno != EWOULDBLOCK) break;
            }
        }
    }

    s_ctx_close(_client);
    return true;
}

uint8_t WiFiClient::connected()
{
    if (!_client || _client->sock < 0) return false;
    if (!_client->connected) return false;

    /* Poll the socket once: error => gone; readable => data or FIN (probe the
     * FIN with a 1-byte MSG_PEEK so a remote close is noticed right away). */
    fd_set rset, eset;
    FD_ZERO(&rset); FD_SET(_client->sock, &rset);
    FD_ZERO(&eset); FD_SET(_client->sock, &eset);
    struct timeval tv = { 0, 0 };
    int rc = lwip_select(_client->sock + 1, &rset, NULL, &eset, &tv);
    if (rc < 0) {
        _client->connected = false;
        return false;
    }
    if (rc > 0) {
        if (FD_ISSET(_client->sock, &eset)) {
            int soerr = 0;
            socklen_t olen = sizeof(soerr);
            if (lwip_getsockopt(_client->sock, SOL_SOCKET, SO_ERROR, &soerr, &olen) == 0 && soerr != 0) {
                _client->connected = false;
                return false;
            }
        }
        if (FD_ISSET(_client->sock, &rset) && !_client->peeked) {
            uint8_t b;
            int prc = lwip_recv(_client->sock, &b, 1, MSG_PEEK);
            if (prc == 0) {
                _client->connected = false;
                return false;
            }
            if (prc == 1) {
                _client->peeked = true;
                _client->peekByte = b;
            }
        }
    }
    return true;
}

WiFiClient::operator bool()
{
    return _client != nullptr && _client->sock >= 0;
}

/* ---- addresses ---- */

IPAddress WiFiClient::remoteIP() { return _client ? _client->remoteIP : IPAddress(); }
uint16_t WiFiClient::remotePort() { return _client ? _client->remotePort : 0; }
IPAddress WiFiClient::localIP() { return _client ? _client->localIP : IPAddress(); }
uint16_t WiFiClient::localPort() { return _client ? _client->localPort : 0; }

int WiFiClient::availableForWrite()
{
    if (!_client || _client->sock < 0 || !_client->connected) return 0;
    /* lwIP ioctl only knows FIONREAD/FIONBIO; report the send-buffer size as
     * the writable space (the reference reports the same idea). */
    return (int)TCP_SND_BUF;
}

/* ---- stopAll ---- */

void WiFiClient::stopAll()
{
    for (std::list<WiFiClientContext*>::iterator it = s_live.begin(); it != s_live.end(); ++it) {
        s_ctx_close(*it);
    }
}

void WiFiClient::stopAllExcept(WiFiClient* exC)
{
    WiFiClientContext* keep = exC ? exC->_client : nullptr;
    for (std::list<WiFiClientContext*>::iterator it = s_live.begin(); it != s_live.end(); ++it) {
        if (*it != keep) s_ctx_close(*it);
    }
}

/* ---- keepalive / nodelay / sync ---- */

void WiFiClient::keepAlive(uint16_t idle_sec, uint16_t intv_sec, uint8_t cnt)
{
    if (!_client) return;
    _client->keepAliveIdleSec = idle_sec;
    _client->keepAliveIntervalSec = intv_sec;
    _client->keepAliveCount = cnt;
    s_apply_keepalive(_client);
}

bool WiFiClient::isKeepAliveEnabled() const { return _client && _client->keepAliveIdleSec != 0; }
uint16_t WiFiClient::getKeepAliveIdle() const { return _client ? _client->keepAliveIdleSec : 0; }
uint16_t WiFiClient::getKeepAliveInterval() const { return _client ? _client->keepAliveIntervalSec : 0; }
uint8_t WiFiClient::getKeepAliveCount() const { return _client ? _client->keepAliveCount : 0; }

void WiFiClient::setDefaultNoDelay(bool noDelay) { s_defaultNoDelay = noDelay; }
bool WiFiClient::getDefaultNoDelay() { return s_defaultNoDelay; }
bool WiFiClient::getNoDelay() const { return _client ? _client->noDelay : s_defaultNoDelay; }
void WiFiClient::setNoDelay(bool nodelay)
{
    if (_client) _client->noDelay = nodelay;
    s_apply_nodelay(_client);
}

void WiFiClient::setDefaultSync(bool sync) { s_defaultSync = sync; }
bool WiFiClient::getDefaultSync() { return s_defaultSync; }
bool WiFiClient::getSync() const { return _client ? _client->sync : s_defaultSync; }
void WiFiClient::setSync(bool sync) { if (_client) _client->sync = sync; }

/* ---- abort ---- */

void WiFiClient::abort()
{
    if (!_client || _client->sock < 0) return;
    struct linger lin;
    lin.l_onoff = 1;
    lin.l_linger = 0;
    lwip_setsockopt(_client->sock, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
    s_ctx_close(_client);
}
