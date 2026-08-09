/*
  SoftwareSerialEcho — bit-banged serial on any two GPIOs.

  Wire: module TX -> GPIO5 (our RX), module RX <- GPIO11 (our TX),
  GND common. Works at 300..115200 baud; a FreeRTOS task samples the RX
  pin at mid-bit into a ring buffer.
*/
#include <SoftwareSerial.h>

SoftwareSerial sw(5, 11);  /* RX, TX */

void setup() {
  Serial.begin(115200);
  sw.begin(9600);
  Serial.println("SoftwareSerial echo: type into the Serial monitor.");
}

void loop() {
  if (sw.available()) {
    Serial.write(sw.read());      /* from the soft serial */
  }
  if (Serial.available()) {
    sw.write(Serial.read());      /* to the soft serial */
  }
}
