/*
  Blink — the classic "hello world" for the Ai-WB2-12F Arduino core.

  Onboard LED is LED_BUILTIN (default GPIO14). If your board's LED is on a
  different GPIO, change LED_BUILTIN in variants/wb2-12f/pins_arduino.h.
*/

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
