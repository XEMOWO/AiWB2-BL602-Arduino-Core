/*
 * I2S.cpp - ESP8266-compatible I2S API for the Ai-WB2-12F (BL602) Arduino core.
 * Stub implementation; see I2S.h. The global instance is what sketches use
 * (I2S.begin / I2S.read / I2S.write).
 */

#include "I2S.h"

I2SClass I2S;

/* ---- legacy core-level I2S C API: benign stubs (see I2S.h) ---- */

bool i2s_rxtx_begin(bool enableRX, bool enableTX)
{
    (void)enableRX; (void)enableTX;
    return true;
}

void i2s_set_rate(uint32_t rate)
{
    (void)rate;
}

void i2s_set_clock(uint32_t divider)
{
    (void)divider;
}

bool i2s_read_sample(int16_t *left, int16_t *right, bool blocking)
{
    (void)blocking;
    if (left) {
        *left = 0;
    }
    if (right) {
        *right = 0;
    }
    return true;
}

bool i2s_write_sample(uint32_t sample)
{
    (void)sample;
    return true;
}

bool i2s_write_sample_nb(uint32_t sample)
{
    (void)sample;
    return true;
}

bool i2s_write_lr(int16_t left, int16_t right)
{
    return i2s_write_sample(((uint32_t)(uint16_t)left << 16) | (uint16_t)right);
}
