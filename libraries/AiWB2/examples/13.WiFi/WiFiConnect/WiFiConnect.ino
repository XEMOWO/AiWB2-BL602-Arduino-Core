/*
  WiFiConnect — connect the Ai-WB2-12F to your router as a station.

  Edit SSID / PASS below. Open the Serial Monitor at 115200; the sketch
  prints the connection status, then the assigned IP once DHCP completes.
*/
#include <Arduino.h>
#include <WiFi.h>

const char *SSID = "YOUR_SSID";
const char *PASS = "YOUR_PASSWORD";

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP:      "); Serial.println(WiFi.localIP().toString());
    Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP().toString());
    Serial.print("Mask:    "); Serial.println(WiFi.subnetMask().toString());
    Serial.print("MAC:     "); Serial.println(WiFi.macAddress());
    Serial.print("RSSI:    "); Serial.println(WiFi.RSSI());
  } else {
    Serial.println("Connection failed.");
    Serial.print("status = ");
    Serial.println((int)WiFi.status());
  }
}

void loop() {
  /* report RSSI every 10 s while connected */
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("RSSI: "); Serial.println(WiFi.RSSI());
  }
  delay(10000);
}
