/*
  StringAndMath — the Arduino compatibility layer.

  String (concatenation, search, replace, toInt/toFloat), F()/PROGMEM
  (no-ops on flash-XIP), and WMath (map, constrain, random).
*/
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(300);

  String name = "Ai-WB2";
  String msg = String("hello ") + name + " #" + String(42);
  msg.toUpperCase();
  Serial.println(msg);                       // HELLO AI-WB2 #42
  Serial.println(msg.indexOf("WB2"));
  Serial.println(msg.substring(6));

  Serial.println(F("F() works too (flash-resident on XIP)."));
  Serial.println(map(128, 0, 255, 0, 100));  // 50
  Serial.println(constrain(300, 0, 255));    // 255

  randomSeed(analogRead(A0));                // seed from the SAR ADC
  Serial.println(random(0, 100));            // 0..99
}

void loop() {
}
