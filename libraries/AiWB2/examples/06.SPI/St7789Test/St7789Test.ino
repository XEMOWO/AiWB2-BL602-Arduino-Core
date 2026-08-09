/*
  St7789Test — drive the XEMOWO_TEST4 ST7789V panel through the hardware SPI
  library. The original project bit-banged the same three wires with GPIO
  software SPI; this sketch reuses that exact wiring via SPI0 at 8 MHz.

  Wiring (identical to XEMOWO_TEST4):
      SCL  -> GPIO3   (SPI SCK)
      SDA  -> GPIO12  (SPI MOSI)
      CS   -> GPIO14
      DC   -> GPIO4
      RST  -> GPIO17  (== the SPI library's default MISO; released to SPI after
                        the one-shot reset below — fine, the panel only needs
                        RST once at power-up)

  The panel is a 320x220 logical window (OY=30 offset on a 320-tall ST7789V).
  After init the sketch cycles solid colors then a red ramp, so whether the
  SPI link works is obvious at a glance. Open the Serial Monitor at 115200.
*/
#include <Arduino.h>
#include <SPI.h>

#define TFT_SCK  3
#define TFT_MOSI 12
#define TFT_CS   14
#define TFT_DC   4
#define TFT_RST  17

#define TFT_W   320
#define TFT_H   220
#define TFT_OY  30    /* original project's panel offset (MADCTL 0x60) */

#define TFT_SPI_FREQ 8000000UL

static uint32_t g_cmd_count = 0;

static void tft_cmd(uint8_t c) {
    if ((g_cmd_count % 5) == 0) {
        Serial.print("  cmd#"); Serial.print(g_cmd_count);
        Serial.print(" SPI.F="); Serial.print(SPI.getFaults());
        Serial.print(" 0x"); Serial.println(c, HEX);
    }
    g_cmd_count++;
    digitalWrite(TFT_DC, LOW);
    digitalWrite(TFT_CS, LOW);
    SPI.transfer(c);
    digitalWrite(TFT_CS, HIGH);
}

static void tft_data(const uint8_t *d, size_t n) {
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    SPI.write(d, n);
    digitalWrite(TFT_CS, HIGH);
}

static void tft_data8(uint8_t d)  { tft_data(&d, 1); }
static void tft_data16(uint16_t v) { uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v }; tft_data(b, 2); }

static void tft_setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    tft_cmd(0x2A); tft_data16(x0);      tft_data16(x1);          /* CASET, OX = 0 */
    tft_cmd(0x2B); tft_data16(y0 + TFT_OY); tft_data16(y1 + TFT_OY); /* RASET + OY */
    tft_cmd(0x2C);                                                /* RAMWR */
}

static void tft_fillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    static uint8_t line[640];            /* 320 px * 2 bytes, MSB first */
    uint32_t i;
    uint16_t w = x1 - x0 + 1;
    uint16_t h = y1 - y0 + 1;

    for (i = 0; i < w; i++) { line[i * 2] = color >> 8; line[i * 2 + 1] = color & 0xFF; }
    tft_setWindow(x0, y0, x1, y1);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    for (i = 0; i < h; i++) {
        SPI.write(line, (size_t)w * 2);
    }
    digitalWrite(TFT_CS, HIGH);
}

/* horizontal ramp in the red channel: one brightness step every 16 columns */
static void tft_redRamp(void) {
    static uint8_t line[640];
    uint32_t i;

    for (i = 0; i < TFT_W; i++) {
        uint16_t c = (uint16_t)(((i >> 4) & 0x1F) << 11);   /* 0x0000 .. 0xF800 */
        line[i * 2] = c >> 8; line[i * 2 + 1] = c & 0xFF;
    }
    tft_setWindow(0, 0, TFT_W - 1, TFT_H - 1);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    for (i = 0; i < TFT_H; i++) {
        SPI.write(line, TFT_W * 2);
    }
    digitalWrite(TFT_CS, HIGH);
}

/* ST7789V init sequence lifted verbatim from XEMOWO_TEST4/main.c */
static void tft_init(void) {
    tft_cmd(0x01); delay(150);          /* SWRESET */
    tft_cmd(0x11); delay(120);          /* SLPOUT  */
    tft_cmd(0x36); tft_data8(0x60);     /* MADCTL  */
    tft_cmd(0x3A); tft_data8(0x55);     /* COLMOD: 16bpp */

    { uint8_t d[] = { 0x0C, 0x0C, 0x00, 0x33, 0x33 }; tft_cmd(0xB2); tft_data(d, sizeof(d)); }
    tft_cmd(0xB7); tft_data8(0x35);
    tft_cmd(0xBB); tft_data8(0x19);
    tft_cmd(0xC0); tft_data8(0x2C);
    tft_cmd(0xC2); tft_data8(0x01);
    tft_cmd(0xC3); tft_data8(0x12);
    tft_cmd(0xC4); tft_data8(0x20);
    tft_cmd(0xC6); tft_data8(0x0F);
    tft_cmd(0xD0); tft_data8(0xA4); tft_data8(0xA1);
    { uint8_t d[] = { 0xD0,0x00,0x02,0x07,0x0A,0x28,0x32,0x44,0x42,0x06,0x0E,0x12,0x14,0x17 };
      tft_cmd(0xE0); tft_data(d, sizeof(d)); }
    { uint8_t d[] = { 0xD0,0x00,0x02,0x07,0x0A,0x28,0x31,0x54,0x47,0x0E,0x1C,0x17,0x1B,0x1E };
      tft_cmd(0xE1); tft_data(d, sizeof(d)); }

    tft_cmd(0x21);                      /* INVON  */
    tft_cmd(0x13);                      /* NORMALON */
    tft_cmd(0x29);                      /* DISPON */
    delay(50);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("ST7789 SPI test DIAG v3 (SPI0 @8MHz polled)");

    Serial.print("step1 reset panel... "); Serial.flush();
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, LOW);
    delay(20);
    digitalWrite(TFT_RST, HIGH);
    delay(120);
    Serial.println("ok");

    Serial.print("step2 SPI.begin... "); Serial.flush();
    SPI.begin(TFT_SCK, TFT_MOSI, TFT_RST, TFT_CS);
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    Serial.println("ok");

    Serial.print("step3 beginTransaction... "); Serial.flush();
    SPI.beginTransaction(SPISettings(TFT_SPI_FREQ, MSBFIRST, SPI_MODE0));
    Serial.println("ok");

    Serial.println("step4 init sequence:"); Serial.flush();
    tft_init();
    Serial.print("step4 done, SPI.F="); Serial.println(SPI.getFaults());

    Serial.println("init done — cycling colors");
}

void loop() {
    tft_fillRect(0, 0, TFT_W - 1, TFT_H - 1, 0xF800); Serial.println("red");   delay(1500);
    tft_fillRect(0, 0, TFT_W - 1, TFT_H - 1, 0x07E0); Serial.println("green"); delay(1500);
    tft_fillRect(0, 0, TFT_W - 1, TFT_H - 1, 0x001F); Serial.println("blue");  delay(1500);
    tft_fillRect(0, 0, TFT_W - 1, TFT_H - 1, 0xFFFF); Serial.println("white"); delay(1500);
    tft_fillRect(0, 0, TFT_W - 1, TFT_H - 1, 0x0000); Serial.println("black"); delay(1500);
    tft_redRamp();                                    Serial.println("red ramp"); delay(3000);
}
