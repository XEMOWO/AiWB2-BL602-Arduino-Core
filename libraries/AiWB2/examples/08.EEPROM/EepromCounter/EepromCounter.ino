/*
  EepromCounter — EEPROM demo for the Ai-WB2-12F Arduino core.

  Each boot increments a counter stored at EEPROM address 0, prints it, then
  commit()s it back to flash. Power-cycle the board and watch it continue from
  the stored value — proof of persistence.

  EEPROM is ESP32-style: write() touches a RAM shadow only; commit() erases
  one 4K sector and writes the shadow to the DATA flash partition. Always call
  commit() before powering off, or the writes are lost.
*/

#include "Arduino.h"
#include <EEPROM.h>

void setup() {
  Serial.begin(115200);
  EEPROM.begin(4096);
  delay(500);

  uint8_t n = EEPROM.read(0);
  if (n == 0xFF) {
    n = 0;               /* fresh flash (erased) reads 0xFF */
  }

  Serial.print("Boot #");
  Serial.println(n + 1);

  EEPROM.write(0, n + 1);
  if (EEPROM.commit()) {
    Serial.println("committed to flash.");
  } else {
    Serial.println("commit failed!");
  }
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
}
