/*
 * NeoPixel.cpp — calibrated NOP-loop bit timing.
 *
 * BL602 runs at 192 MHz (5.2 ns/cycle). The loop below ("addi; bnez") costs
 * 2 cycles per iteration. The pulse counts are calibrated for the WS2812B
 * 800 kHz timings (T0H 350 ns, T1H 700 ns, bit low ~700 ns, reset >=50 us);
 * 400 kHz (WS2811) uses twice the counts. If a particular clone is picky,
 * adjust NP_800_* / NP_400_* at the top of this file.
 *
 * show() runs with interrupts disabled so the FreeRTOS tick / UART ISRs
 * cannot stretch a pulse.
 */
#include "NeoPixel.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
#include <bl_gpio.h>
}

/* NOP-loop pulse counts (2 cycles/iteration, 192 MHz). */
#define NP_800_H1 62  /* ~0.72 us  high time of a 1 bit  */
#define NP_800_H0 28  /* ~0.35 us  high time of a 0 bit  */
#define NP_800_LOW 68 /* ~0.72 us  low time after either */
#define NP_400_H1 125
#define NP_400_H0 55
#define NP_400_LOW 150

#define NP_DELAY(iters)                          \
    do {                                         \
        uint32_t _n = (iters);                   \
        __asm__ __volatile__(                    \
            "1: addi %0,%0,-1\n"                 \
            "   bnez %0,1b\n"                    \
            : "+r"(_n)                           \
            :                                    \
            : "memory");                         \
    } while (0)

/* Channel emission order indexed by the low nibble of the type flag. */
static const uint8_t NP_ORDER[22][4] = {
    {0, 1, 2, 3}, {0, 2, 1, 3}, {1, 0, 2, 3}, {1, 2, 0, 3}, /* RGB RBG GRB GBR */
    {2, 0, 1, 3}, {2, 1, 0, 3}, {3, 0, 1, 2}, {0, 1, 2, 3}, /* BRG BGR WRGB RGBW */
    {3, 0, 2, 1}, {0, 3, 2, 1}, {1, 3, 0, 2}, {1, 0, 3, 2}, /* WRBG RWBG GWRB GRWB */
    {1, 2, 3, 0}, {1, 3, 2, 0}, {2, 3, 0, 1}, {2, 0, 3, 1}, /* GBWR GWBR BWRG BRWG */
    {2, 1, 3, 0}, {2, 0, 1, 3}, {3, 2, 0, 1}, {3, 2, 1, 0}, /* BGWR BRGW WBRG WBGR */
    {3, 1, 0, 2}, {3, 1, 2, 0}                              /* WGRB WGBR */
};

Adafruit_NeoPixel::Adafruit_NeoPixel(uint16_t n, uint8_t pin, uint16_t type)
    : _numLEDs(n), _pin(pin), _type(type), _brightness(255)
{
    _pixels = (uint8_t *)malloc((size_t)n * 4); /* RGBW, 4 bytes/pixel */
    if (_pixels) {
        memset(_pixels, 0, (size_t)n * 4);
    }
}

Adafruit_NeoPixel::~Adafruit_NeoPixel()
{
    if (_pixels) {
        free(_pixels);
        _pixels = NULL;
    }
}

void Adafruit_NeoPixel::begin(void)
{
    bl_gpio_enable_output(_pin, 0, 0);
    bl_gpio_output_set(_pin, 0);
}

void Adafruit_NeoPixel::clear(void)
{
    if (_pixels) {
        memset(_pixels, 0, (size_t)_numLEDs * 4);
    }
}

void Adafruit_NeoPixel::fill(uint32_t c, uint16_t first, uint16_t count)
{
    uint16_t i, last;

    if (first >= _numLEDs) {
        return;
    }
    last = (count == 0 || first + count > _numLEDs) ? _numLEDs : first + count;
    for (i = first; i < last; i++) {
        setPixelColor(i, c);
    }
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
    setPixelColor(n, r, g, b, 0);
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g,
                                      uint8_t b, uint8_t w)
{
    if (n >= _numLEDs || !_pixels) {
        return;
    }
    _pixels[(size_t)n * 4 + 0] = r;
    _pixels[(size_t)n * 4 + 1] = g;
    _pixels[(size_t)n * 4 + 2] = b;
    _pixels[(size_t)n * 4 + 3] = w;
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint32_t c)
{
    if (n >= _numLEDs || !_pixels) {
        return;
    }
    _pixels[(size_t)n * 4 + 0] = (c >> 16) & 0xFF; /* R */
    _pixels[(size_t)n * 4 + 1] = (c >> 8) & 0xFF;  /* G */
    _pixels[(size_t)n * 4 + 2] = c & 0xFF;         /* B */
    _pixels[(size_t)n * 4 + 3] = (c >> 24) & 0xFF; /* W */
}

uint32_t Adafruit_NeoPixel::getPixelColor(uint16_t n) const
{
    if (n >= _numLEDs || !_pixels) {
        return 0;
    }
    return ((uint32_t)_pixels[(size_t)n * 4 + 3] << 24) |
           ((uint32_t)_pixels[(size_t)n * 4 + 0] << 16) |
           ((uint32_t)_pixels[(size_t)n * 4 + 1] << 8) |
           _pixels[(size_t)n * 4 + 2];
}

uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) const
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) const
{
    return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void Adafruit_NeoPixel::_emitByte(uint8_t b, uint8_t khz400)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        if (b & 0x80) {
            bl_gpio_output_set(_pin, 1);
            NP_DELAY(khz400 ? NP_400_H1 : NP_800_H1);
            bl_gpio_output_set(_pin, 0);
            NP_DELAY(khz400 ? NP_400_LOW : NP_800_LOW);
        } else {
            bl_gpio_output_set(_pin, 1);
            NP_DELAY(khz400 ? NP_400_H0 : NP_800_H0);
            bl_gpio_output_set(_pin, 0);
            NP_DELAY(khz400 ? NP_400_LOW : NP_800_LOW);
        }
        b <<= 1;
    }
}

void Adafruit_NeoPixel::_emitPixel(uint16_t n)
{
    const uint8_t *order = NP_ORDER[_type & 0x0F];
    const uint8_t *px = &_pixels[(size_t)n * 4];
    uint8_t khz400 = (_type & NEO_KHZ400) ? 1 : 0;
    uint8_t channels = _isRGBW() ? 4 : 3;
    uint8_t k;

    for (k = 0; k < channels; k++) {
        uint8_t v = px[order[k]];
        if (_brightness < 255) {
            v = (uint16_t)v * _brightness >> 8;
        }
        _emitByte(v, khz400);
    }
}

void Adafruit_NeoPixel::show(void)
{
    uint16_t n;

    if (!_pixels) {
        return;
    }
    noInterrupts(); /* pulses must not be stretched by ISRs */
    for (n = 0; n < _numLEDs; n++) {
        _emitPixel(n);
    }
    bl_gpio_output_set(_pin, 0);
    delayMicroseconds(80); /* >50 us reset */
    interrupts();
}
