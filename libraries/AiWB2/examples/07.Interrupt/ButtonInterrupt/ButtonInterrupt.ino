/*
  ButtonInterrupt — attachInterrupt demo for the Ai-WB2-12F Arduino core.

  Wire a button between GPIO11 and GND. INPUT_PULLUP enables the internal
  pull-up; every press fires the ISR on the falling edge and bumps a counter
  that loop() prints. GPIO11 is ADC-capable but unused by the other demos.

  ISR rules: keep it short — no delay(), no Serial prints, just set a
  volatile flag/counter and service it in loop().
*/

#include "Arduino.h"

#define BUTTON_PIN 11

volatile unsigned long pressCount = 0;

void onButton() {
  pressCount++;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButton, FALLING);
  delay(500);
  Serial.println("ButtonInterrupt: press the button on GPIO11...");
}

void loop() {
  if (pressCount != 0) {
    Serial.print("presses: ");
    Serial.println(pressCount);
    delay(200);          /* simple print-rate limiter, not a debounce */
  }
}
