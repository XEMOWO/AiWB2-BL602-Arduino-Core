/*
  RGBLoop — cycle a WS2812 strip through colors on GPIO14.

  Wire: strip DIN -> GPIO14 (any GPIO works), strip VCC -> 5V, GND -> GND.
  For longer strips add a ~470 ohm series resistor on DIN and a 100-1000 uF
  capacitor across the strip's power pins.
*/
#include <NeoPixel.h>

#define NUMPIXELS 8
Adafruit_NeoPixel strip(NUMPIXELS, 14, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(64);       /* keep it dim for a demo */
}

void loop() {
  static uint8_t h = 0;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, strip.Color(h, 255 - h, (h * 3) & 0xFF));
  }
  strip.show();
  h++;
  delay(20);
}
