/*
  WiFiClient.h - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino.  All right reserved.

  Ported to Ai-WB2-12F (BL602): backend is the SDK's lwIP socket API. The
  internal WiFiClientContext holds one socket; copies of a WiFiClient share
  that context (refcounted), so `WiFiClient c = server.accept();` and passing
  a client by value behave like the reference ClientContext. The last copy to
  die closes the connection.

  lwIP builds with LWIP_COMPAT_SOCKETS==1 (the POSIX socket names are macros),
  so lwip_compat_undef.h is pulled in before the class declares connect/read/
  write and this library always calls the explicit lwip_* functions.
 */
#ifndef wificlient_h
#define wificlient_h

#include "lwip_compat_undef.h"

#include <memory>
#include "Arduino.h"
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"

#ifndef TCP_MSS
#define TCP_MSS 1460 // lwip1.4
#endif

#define WIFICLIENT_MAX_PACKET_SIZE TCP_MSS
#define WIFICLIENT_MAX_FLUSH_WAIT_MS 300

#define TCP_DEFAULT_KEEPALIVE_IDLE_SEC          7200 // 2 hours
#define TCP_DEFAULT_KEEPALIVE_INTERVAL_SEC      75   // 75 sec
#define TCP_DEFAULT_KEEPALIVE_COUNT             9    // fault after 9 failures

class WiFiServer;

class WiFiClient : public Client {
protected:
  WiFiClient(int sock);   /* wraps an already-connected socket (WiFiServer::accept) */

public:
  struct WiFiClientContext;   /* shared socket state, defined in WiFiClient.cpp.
                               * Public so the socket helpers (and WiFiServer)
                               * can name the type; never dereferenced by users. */

  WiFiClient();
  virtual ~WiFiClient();
  WiFiClient(const WiFiClient&);
  WiFiClient& operator=(const WiFiClient&);

  virtual std::unique_ptr<WiFiClient> clone() const;

  virtual uint8_t status();
  virtual int connect(IPAddress ip, uint16_t port) override;
  virtual int connect(const char *host, uint16_t port) override;
  virtual int connect(const String& host, uint16_t port);
  virtual size_t write(uint8_t) override;
  virtual size_t write(const uint8_t *buf, size_t size) override;
  virtual size_t write_P(PGM_P buf, size_t size);
  __attribute__((deprecated("use stream.sendHow(client...)")))
  size_t write(Stream& stream);

  virtual int available() override;
  virtual int read() override;
  virtual int read(uint8_t* buf, size_t size) override;
  int read(char* buf, size_t size);

  virtual int peek() override;
  virtual size_t peekBytes(uint8_t *buffer, size_t length);
  size_t peekBytes(char *buffer, size_t length) {
    return peekBytes((uint8_t *) buffer, length);
  }
  virtual void flush() override { (void)flush(0); } // wait for all outgoing characters to be sent, output buffer should be empty after this call
  virtual void stop() override { (void)stop(0); }
  bool flush(unsigned int maxWaitMs);
  bool stop(unsigned int maxWaitMs);
  virtual uint8_t connected() override;
  virtual operator bool() override;

  virtual IPAddress remoteIP();
  virtual uint16_t  remotePort();
  virtual IPAddress localIP();
  virtual uint16_t  localPort();

  static void setLocalPortStart(uint16_t port) { _localPort = port; }

  int availableForWrite() override;

  friend class WiFiServer;

  using Print::write;

  static void stopAll();
  static void stopAllExcept(WiFiClient * c);

  virtual void     keepAlive (uint16_t idle_sec = TCP_DEFAULT_KEEPALIVE_IDLE_SEC, uint16_t intv_sec = TCP_DEFAULT_KEEPALIVE_INTERVAL_SEC, uint8_t count = TCP_DEFAULT_KEEPALIVE_COUNT);
  virtual bool     isKeepAliveEnabled () const;
  virtual uint16_t getKeepAliveIdle () const;
  virtual uint16_t getKeepAliveInterval () const;
  virtual uint8_t  getKeepAliveCount () const;
  virtual void     disableKeepAlive () { keepAlive(0, 0, 0); }

  // default NoDelay=False (Nagle=True=!NoDelay)
  static void setDefaultNoDelay (bool noDelay);
  static bool getDefaultNoDelay ();
  bool getNoDelay() const;
  void setNoDelay(bool nodelay);

  // default Sync=false
  static void setDefaultSync (bool sync);
  static bool getDefaultSync ();
  bool getSync() const;
  void setSync(bool sync);

  // peek buffer API is present
  virtual bool hasPeekBufferAPI () const override;

  // return number of byte accessible by peekBuffer()
  virtual size_t peekAvailable () override;

  // return a pointer to available data buffer (size = peekAvailable())
  virtual const char* peekBuffer () override;

  // consume bytes after use (see peekBuffer)
  virtual void peekConsume (size_t consume) override;

  virtual bool outputCanTimeout () override { return connected(); }
  virtual bool inputCanTimeout () override { return connected(); }

  // Immediately stops this client instance.
  // Unlike stop(), does not wait to gracefully shutdown the connection.
  void abort();

protected:
  WiFiClientContext* _client;  /* refcounted; copies share this */
  static uint16_t _localPort;
};

#endif
