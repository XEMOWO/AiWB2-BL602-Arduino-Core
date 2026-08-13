/*
  Serial1Echo — second UART on UART1 (default GPIO11 TX / GPIO17 RX).

  Serial  = UART0 (GPIO16 TX / GPIO7 RX @115200, the SDK console).
  Serial1 = UART1 (default GPIO11 TX / GPIO17 RX). Short the two pins
  together (plus a common GND) and every byte typed into the monitor is
  echoed out of Serial1 and back through the loopback.

  begin(baud) uses the default pins; begin(baud, tx, rx) remaps to any
  UART1-muxable GPIO.
*/
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);            // default GPIO11 TX / GPIO17 RX
  // Serial1.begin(9600, 11, 17);   // explicit pins
  Serial.println("Serial1 echo: connect GPIO11 TX to GPIO17 RX.");
}

void loop() {
  if (Serial.available()) {
    int c = Serial.read();
    Serial1.write((uint8_t)c);      // out of Serial1
    Serial.write((uint8_t)c);       // local echo
  }
  if (Serial1.available()) {
    Serial.write(Serial1.read());   // back from the loopback
  }
}
