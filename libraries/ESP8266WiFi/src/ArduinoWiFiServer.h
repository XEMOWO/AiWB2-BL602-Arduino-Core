/*
  ArduinoWiFiServer.h - Arduino compatible WiFiServer implementation
  for the ESP8266 core.  Copyright (c) 2020 Juraj Andrassy.

  Ported to the Ai-WB2-12F (BL602) Arduino core unchanged: it layers a
  monitored-client pool over WiFiServer so available() behaves as Arduino's
  EthernetServer documents (return the first client with pending data).

  PagerServer (ESP8266WiFi/examples/PagerServer) instantiates `ArduinoWiFiServer`.
  The secure variant (ArduinoWiFiServerSecure) is defined once the BearSSL
  WiFiServerSecure/WiFiClientSecure classes land.
*/

#ifndef arduinowifiserver_h
#define arduinowifiserver_h

#include <ESP8266WiFi.h>

/* lwIP TCP states as used by WiFiClient::status() / WiFiServer::status().
 * Guarded so lwip/tcp.h wins when it is included. */
#ifndef ESTABLISHED
#define ESTABLISHED 4
#endif
#ifndef LISTEN
#define LISTEN 1
#endif

#ifndef MAX_MONITORED_CLIENTS
#define MAX_MONITORED_CLIENTS 5
#endif

template <class TServer, class TClient>
class ArduinoCompatibleWiFiServerTemplate : public TServer, public Print {
public:

  ArduinoCompatibleWiFiServerTemplate(const IPAddress& addr, uint16_t port) : TServer(addr, port) {}
  ArduinoCompatibleWiFiServerTemplate(uint16_t port = 23) : TServer(port) {}
  virtual ~ArduinoCompatibleWiFiServerTemplate() {}

  TClient available() {

    acceptClients();

    for (uint8_t i = 0; i < MAX_MONITORED_CLIENTS; i++) {
      if (index == MAX_MONITORED_CLIENTS) {
        index = 0;
      }
      TClient& client = connectedClients[index];
      index++;
      if (client.available())
        return client;
    }
    return TClient(); // no client with data found
  }

  virtual size_t write(uint8_t b) override {
    return write(&b, 1);
  }

  virtual size_t write(const uint8_t *buf, size_t size) override {
    static uint32_t lastCheck;
    uint32_t m = millis();
    if (m - lastCheck > 100) {
      lastCheck = m;
      acceptClients();
    }
    if (size == 0)
      return 0;
    size_t ret = 0;
    size_t a = size;
    while (true) {
      for (uint8_t i = 0; i < MAX_MONITORED_CLIENTS; i++) {
        WiFiClient& client = connectedClients[i];
        if (client.status() == ESTABLISHED && client.availableForWrite() < (int) a) {
          a = client.availableForWrite();
        }
      }
      if (a == 0)
        break;
      for (uint8_t i = 0; i < MAX_MONITORED_CLIENTS; i++) {
        if (connectedClients[i].status() == ESTABLISHED) {
          connectedClients[i].write(buf, a);
        }
      }
      ret += a;
      if (ret == size)
        break;
      buf += a;
      a = size - ret;
    }
    return ret;
  }

  using Print::write;

  virtual void flush() override {
    flush(0);
  }

  virtual void flush(unsigned int maxWaitMs) {
    for (uint8_t i = 0; i < MAX_MONITORED_CLIENTS; i++) {
      if (connectedClients[i].status() == ESTABLISHED) {
        connectedClients[i].flush(maxWaitMs);
      }
    }
  }

  operator bool() {
    return (TServer::status() == LISTEN);
  }

  void close() {
    TServer::stop();
    for (uint8_t i = 0; i < MAX_MONITORED_CLIENTS; i++) {
      if (connectedClients[i]) {
        connectedClients[i].stop();
      }
    }
  }
  void stop() {close();}
  void end() {close();}

private:
  TClient connectedClients[MAX_MONITORED_CLIENTS];
  uint8_t index = 0;

  void acceptClients() {
    for (uint8_t i = 0; i < MAX_MONITORED_CLIENTS; i++) {
      if (!connectedClients[i]) {
        connectedClients[i] = TServer::accept();
      }
    }
  }
};

typedef ArduinoCompatibleWiFiServerTemplate<WiFiServer, WiFiClient> ArduinoWiFiServer;

#endif
