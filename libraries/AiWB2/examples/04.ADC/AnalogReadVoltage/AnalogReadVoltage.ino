/*
  AnalogReadVoltage — ADC demo (analogRead) for the Ai-WB2-12F Arduino core.

  Reads A0 (= GPIO12, ADC ch0) and prints the 0..4095 raw value plus the
  voltage in millivolts (3.2V full scale). Board-level ADC pins:

    A0=12  A1=4  A2=14  A3=5  A4=11

  The same GPIO can also be read by raw number: analogRead(12).

  Wire a potentiometer (wiper to GPIO12, ends to 3V3 and GND) and the printed
  value tracks the knob.
*/

#include "Arduino.h"

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("AnalogReadVoltage: reading A0 (= GPIO12)...");
}

void loop() {
  int raw = analogRead(A0);
  long mv = (long)raw * 3200 / 4095;   /* back out the mV @ 3.2V scale */

  Serial.print("A0 raw=");
  Serial.print(raw);
  Serial.print("  ~");
  Serial.print(mv);
  Serial.println(" mV");
  delay(500);
}
