/*
  WiFiServer.h - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

  Ported to Ai-WB2-12F (BL602): backend is the SDK's lwIP socket API. Like the
  reference, this class is standalone (it does NOT derive from the minimal
  Server base). Incoming connections wait in the kernel listen backlog and are
  claimed by accept()/available(); hasClient() polls non-destructively.

  lwIP COMPAT_SOCKETS macros are neutralized before the class declares accept/
  close (see lwip_compat_undef.h); the socket calls use the lwip_* functions.
 */
#ifndef wifiserver_h
#define wifiserver_h

#include "lwip_compat_undef.h"

#include <Server.h>
#include <IPAddress.h>

#include "WiFiClient.h"

class WiFiServer {
  // Secure server needs access to all the private entries here
protected:
  uint16_t _port;
  IPAddress _addr;
  int _listen_fd;
  enum { _ndDefault, _ndFalse, _ndTrue } _noDelay;

public:
  WiFiServer(const IPAddress& addr, uint16_t port);
  WiFiServer(uint16_t port = 23);
  virtual ~WiFiServer();

  WiFiClient accept(); // https://www.arduino.cc/en/Reference/EthernetServerAccept
  WiFiClient available(uint8_t* status = NULL) __attribute__((deprecated("Renamed to accept().")));
  bool hasClient();
  // hasClientData(): returns the amount of data available from the first
  // client, or 0 if there is none.
  size_t hasClientData();
  // hasMaxPendingClients(): returns true if the queue of pending clients is
  // full (lwIP's kernel backlog handles this internally, so we report false).
  bool hasMaxPendingClients();

  void begin();
  void begin(uint16_t port);
  void begin(uint16_t port, uint8_t backlog);
  void setNoDelay(bool nodelay);
  bool getNoDelay();
  uint8_t status();
  uint16_t port() const;
  void close();
  void stop();
  void end();
  explicit operator bool();

  using ClientType = WiFiClient;
};

#endif
