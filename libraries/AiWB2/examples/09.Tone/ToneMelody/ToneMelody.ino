/*
  ToneMelody — tone()/noTone() demo for the Ai-WB2-12F Arduino core.

  Plays a little melody on a piezo buzzer connected to GPIO4 (any GPIO works).

  tone() bit-bangs a square wave from a high-priority task, so it pins the
  CPU while playing — keep each note short (duration argument).
*/

#include "Arduino.h"

#define BUZZER_PIN 4

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  delay(500);
  Serial.println("ToneMelody: buzzer on GPIO4...");
}

void loop() {
  tone(BUZZER_PIN, 262, 250);  delay(300);   /* C4 */
  tone(BUZZER_PIN, 330, 250);  delay(300);   /* E4 */
  tone(BUZZER_PIN, 392, 250);  delay(300);   /* G4 */
  tone(BUZZER_PIN, 523, 500);  delay(600);   /* C5 */
  noTone(BUZZER_PIN);
  delay(1000);
}
