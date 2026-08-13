/*
  ScanAndProbe — I2C bus scan (Wire) for the Ai-WB2-12F Arduino core.

  Scans 7-bit addresses 0x03..0x77 on the default I2C pins:
    SCL=GPIO12  SDA=GPIO3   (see pins_arduino.h, 100 kHz)
  Wire up any I2C part (e.g. an SHT31) and it will show up in the list.
  To use other pins: Wire.begin(sda, scl).

  NOTE: the default SPI pins reuse GPIO3/12 — don't run SPI with the defaults
  at the same time (pick explicit pins for one of the two buses).
*/

#include "Arduino.h"
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();            /* SCL=12, SDA=3 @100 kHz */
  delay(500);
  Serial.println("ScanAndProbe: scanning I2C bus 0x03..0x77...");
}

void loop() {
  byte error, addr;
  int n = 0;

  Serial.println("I2C scan:");
  for (addr = 0x03; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("  0x");
      Serial.print(addr, HEX);
      Serial.println("  <-- found");
      n++;
    }
  }
  Serial.print("done, ");
  Serial.print(n);
  Serial.println(" device(s).");
  delay(2000);
}
