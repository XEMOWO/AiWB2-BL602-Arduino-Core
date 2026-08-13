/*
  IRSender — blast a NEC power code at 38 kHz.

  Wire: IR LED + ~100 ohm series resistor -> GPIO4 (PWM-capable, CH4);
  use a transistor driver for longer range. IRreceive decodes on the
  matching side.
*/
#include <IRremote.h>

IRsend irsend(4);

void setup() {
  Serial.begin(115200);
  irsend.begin();
  Serial.println("sending NEC 0x00FF9867 every 2 s");
}

void loop() {
  irsend.sendNEC(0x00FF9867UL);  /* a typical "power" code */
  delay(2000);
}
