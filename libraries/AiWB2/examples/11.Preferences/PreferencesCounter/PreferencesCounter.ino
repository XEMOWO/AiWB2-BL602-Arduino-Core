/*
  PreferencesCounter — ESP32-style key-value storage.

  Preferences keeps a small table in its own 4K sector of the DATA flash
  partition (separate from the EEPROM library's sector). The put and get
  functions update a RAM shadow; end() (or commit()) persists it. Power-cycle
  the board and the counter keeps counting.
*/
#include <Arduino.h>
#include <Preferences.h>

Preferences prefs;

void setup() {
  Serial.begin(115200);
  delay(300);

  prefs.begin("counter", false);
  int32_t n = prefs.getInt("boot", 0) + 1;
  prefs.putInt("boot", n);
  prefs.putString("chip", "wb2");
  Serial.printf("Boot #%d, name=%s, free=%u\n",
                (int)n,
                prefs.getString("name", "none").c_str(),
                (unsigned)prefs.freeEntries());
  prefs.end();                       /* persists */
}

void loop() {
}
