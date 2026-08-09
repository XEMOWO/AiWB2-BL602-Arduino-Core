/*
  PwmFade — PWM demo (analogWrite) for the Ai-WB2-12F Arduino core.

  Fades the onboard LED (LED_BUILTIN = GPIO14, PWM CH4) up and down.

  PWM channel = GPIO % 5, so some pins share a channel:
    CH2 = {12, 17, 7}   CH4 = {14, 4}
  analogWrite() on a second pin of an active channel steals it (the previous
  pin stops). GPIO7/16 are the Serial UART — never PWM them.
  A 4.7k pull-down on PWM output pins is recommended per the spec (avoids a
  flash of light at power-on).
*/

#include "Arduino.h"

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("PwmFade: fading LED_BUILTIN (GPIO14, PWM CH4)...");
}

void loop() {
  for (int v = 0; v <= 255; v++) {
    analogWrite(LED_BUILTIN, v);
    delay(4);
  }
  for (int v = 255; v >= 0; v--) {
    analogWrite(LED_BUILTIN, v);
    delay(4);
  }
}
