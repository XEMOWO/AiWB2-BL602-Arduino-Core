/*
 * NeoPixel.h — WS2812/WS2811 driver, Adafruit_NeoPixel-compatible subset.
 *
 * The bit stream is generated with calibrated NOP loops (192 MHz); a normal
 * delayMicroseconds() is too coarse for the ~350 ns / ~800 ns WS2812 pulse
 * widths. show() disables interrupts for the whole frame so timing holds.
 *
 * Color orders: the low 4 bits of the type select the channel order (6 RGB +
 * 16 RGBW variants); bit 4 selects 400 kHz vs 800 kHz.
 */
#ifndef NeoPixel_h
#define NeoPixel_h

#include <Arduino.h>

class Adafruit_NeoPixel
{
public:
    Adafruit_NeoPixel(uint16_t n, uint8_t pin,
                      uint16_t type = NEO_GRB | NEO_KHZ800);
    ~Adafruit_NeoPixel();

    void begin(void);
    void show(void);            /* push the pixel buffer to the strip */
    void clear(void);
    void fill(uint32_t c, uint16_t first = 0, uint16_t count = 0);

    void setPin(uint8_t pin) { _pin = pin; }

    /* one channel */
    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void setPixelColor(uint16_t n, uint32_t c);
    uint32_t getPixelColor(uint16_t n) const;

    uint32_t Color(uint8_t r, uint8_t g, uint8_t b) const;
    uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) const;

    uint16_t numPixels(void) const { return _numLEDs; }
    void setBrightness(uint8_t b) { _brightness = b; }
    uint8_t getBrightness(void) const { return _brightness; }

    /* type flags, matching Adafruit_NeoPixel */
    enum NeoPixelType {
        NEO_RGB  = 0x00, NEO_RBG  = 0x01, NEO_GRB  = 0x02, NEO_GBR = 0x03,
        NEO_BRG  = 0x04, NEO_BGR  = 0x05,
        NEO_WRGB = 0x06, NEO_RGBW = 0x07, NEO_WRBG = 0x08, NEO_RWBG = 0x09,
        NEO_GWRB = 0x0A, NEO_GRWB = 0x0B, NEO_GBWR = 0x0C, NEO_GWBR = 0x0D,
        NEO_BWRG = 0x0E, NEO_BRWG = 0x0F, NEO_BGWR = 0x10, NEO_BRGW = 0x11,
        NEO_WBRG = 0x12, NEO_WBGR = 0x13, NEO_WGRB = 0x14, NEO_WGBR = 0x15,
        NEO_KHZ400 = 0x10, NEO_KHZ800 = 0x00
    };

private:
    uint16_t _numLEDs;
    uint8_t  _pin;
    uint16_t _type;
    uint8_t *_pixels; /* 4 bytes per pixel: RGBW */
    uint8_t  _brightness;

    bool _isRGBW(void) const { return (_type & 0x0F) >= NEO_WRGB; }
    void _emitByte(uint8_t b, uint8_t khz400);
    void _emitPixel(uint16_t n); /* emit one pixel in the configured order */
};

/* Global aliases so sketches can write bare NEO_GRB + NEO_KHZ800 at file
 * scope, exactly like the Adafruit examples. */
enum {
    NEO_RGB   = Adafruit_NeoPixel::NEO_RGB,
    NEO_RBG   = Adafruit_NeoPixel::NEO_RBG,
    NEO_GRB   = Adafruit_NeoPixel::NEO_GRB,
    NEO_GBR   = Adafruit_NeoPixel::NEO_GBR,
    NEO_BRG   = Adafruit_NeoPixel::NEO_BRG,
    NEO_BGR   = Adafruit_NeoPixel::NEO_BGR,
    NEO_WRGB  = Adafruit_NeoPixel::NEO_WRGB,
    NEO_RGBW  = Adafruit_NeoPixel::NEO_RGBW,
    NEO_WRBG  = Adafruit_NeoPixel::NEO_WRBG,
    NEO_RWBG  = Adafruit_NeoPixel::NEO_RWBG,
    NEO_GWRB  = Adafruit_NeoPixel::NEO_GWRB,
    NEO_GRWB  = Adafruit_NeoPixel::NEO_GRWB,
    NEO_GBWR  = Adafruit_NeoPixel::NEO_GBWR,
    NEO_GWBR  = Adafruit_NeoPixel::NEO_GWBR,
    NEO_BWRG  = Adafruit_NeoPixel::NEO_BWRG,
    NEO_BRWG  = Adafruit_NeoPixel::NEO_BRWG,
    NEO_BGWR  = Adafruit_NeoPixel::NEO_BGWR,
    NEO_BRGW  = Adafruit_NeoPixel::NEO_BRGW,
    NEO_WBRG  = Adafruit_NeoPixel::NEO_WBRG,
    NEO_WBGR  = Adafruit_NeoPixel::NEO_WBGR,
    NEO_WGRB  = Adafruit_NeoPixel::NEO_WGRB,
    NEO_WGBR  = Adafruit_NeoPixel::NEO_WGBR,
    NEO_KHZ800 = Adafruit_NeoPixel::NEO_KHZ800,
    NEO_KHZ400 = Adafruit_NeoPixel::NEO_KHZ400
};

#endif /* NeoPixel_h */
