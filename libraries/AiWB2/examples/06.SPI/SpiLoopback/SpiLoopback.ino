/*
  SpiLoopback — SPI demo for the Ai-WB2-12F Arduino core.

  Jumper MOSI (GPIO12) to MISO (GPIO17), run, and every byte sent should come
  back identical (loopback). Default pins:
    SCK=GPIO3  MOSI=GPIO12  MISO=GPIO17  CS=GPIO4   (see pins_arduino.h)

  To talk to a real slave, pull CS low during the transaction — begin/end
  Transaction already does that for you.

  NOTE: the default I2C pins reuse GPIO12/3 — don't run Wire with the defaults
  at the same time (pick explicit pins for one of the two buses).
*/

#include "Arduino.h"
#include <SPI.h>

void setup() {
  Serial.begin(115200);
  SPI.begin();            /* SCK=3, MOSI=12, MISO=17, CS=4 */
  delay(500);
  Serial.println("SpiLoopback: jumper MOSI(12) to MISO(17)...");
}

void loop() {
  byte tx = 0x55, rx;

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  rx = SPI.transfer(tx);
  SPI.endTransaction();

  Serial.print("tx=0x");
  Serial.print(tx, HEX);
  Serial.print("  rx=0x");
  Serial.print(rx, HEX);
  Serial.println(rx == tx ? "  OK (loopback)" : "  MISMATCH");
  delay(500);
}
