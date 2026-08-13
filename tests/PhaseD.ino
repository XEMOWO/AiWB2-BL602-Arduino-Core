/*
  PhaseD.ino - Phase D acceptance build.
  Exercises the ESP8266WiFi library API surface: STA mode, scan, TCP client,
  UDP, WiFiMulti and time (configTime -> sntp). Compiling + linking this sketch
  against the Ai-WB2-12F core proves the whole network stack links; running it
  on hardware exercises the lwIP socket backend.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <time.h>

const char* ssid = "test-ssid";
const char* password = "test-password";

WiFiServer server(80);
WiFiClient client;
WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  Serial.printf("Mode: %d\n", (int)WiFi.getMode());

  // Scan API
  int8_t n = WiFi.scanNetworks();
  Serial.printf("scan: %d networks\n", (int)n);
  for (int8_t i = 0; i < n && i < 10; i++) {
    String ssid;
    uint8_t enc;
    int32_t rssi;
    uint8_t* bssid;
    int32_t channel;
    bool hidden;
    WiFi.getNetworkInfo(i, ssid, enc, rssi, bssid, channel, hidden);
    Serial.printf("  %d: %s (%d dBm)\n", (int)i, ssid.c_str(), (int)rssi);
  }
  WiFi.scanDelete();

  // STA connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }
  Serial.printf("connected, IP %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("gw %s mask %s dns %s\n",
                WiFi.gatewayIP().toString().c_str(),
                WiFi.subnetMask().toString().c_str(),
                WiFi.dnsIP().toString().c_str());

  // TCP client (HTTP GET via the socket backend)
  WiFiClient c;
  if (c.connect("example.com", 80)) {
    c.println("GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n");
    uint32_t t0 = millis();
    while (c.available() == 0 && millis() - t0 < 5000) delay(10);
    while (c.available()) {
      Serial.write(c.read());
    }
    c.stop();
  }

  // WiFiMulti
  ESP8266WiFiMulti wm;
  wm.addAP("multi1", "pw1");
  wm.addAP(ssid, password);
  if (wm.run(10000) == WL_CONNECTED) {
    Serial.println("WiFiMulti connected");
  }

  // UDP
  udp.begin(8888);
  udp.beginPacket("8.8.8.8", 53);
  udp.write((const uint8_t*)"\x12\x34\x01\x00", 4);
  udp.endPacket();
  udp.stop();

  // time (sntp via configTime)
  configTime(8 * 3600, 0, "ntp.aliyun.com");
  delay(100);
  time_t t = time(nullptr);
  Serial.printf("epoch: %d\n", (int)t);
}

void loop() {
  delay(10000);
}
