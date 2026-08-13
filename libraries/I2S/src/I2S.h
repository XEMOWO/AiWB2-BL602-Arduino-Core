/*
 * I2S.h - ESP8266-compatible I2S API for the Ai-WB2-12F (BL602) Arduino core.
 *
 * Compile-compatible port. The BL602 has an I2S peripheral (hosal_i2s_*) but a
 * real driver needs board pin wiring and hardware validation; until that lands
 * the class is a benign stub: begin() reports success, read() returns silence
 * (0), write() accepts samples and discards them. Sketches therefore compile
 * and run without crashing.
 *
 * The API mirrors the ESP8266 core's I2S library (libraries/I2S).
 */

#ifndef I2S_H
#define I2S_H

#include <Arduino.h>

enum I2SMode {
    I2S_PHILIPS_MODE = 0,            // common on ESP8266 boards
    I2S_RIGHT_JUSTIFIED_MODE = 1,
    I2S_LEFT_JUSTIFIED_MODE = 2,
    I2S_DSP_MODE = 3,
    I2S_PHILIPS_MODE_LJ = 4,
    I2S_PHILIPS_MODE_RJ = 5,
    I2S_DSP_MODE_SHORT = 6,
    I2S_LEFT_JUSTIFIED_MODE_LJ = 7
};

class I2SClass
{
public:
    I2SClass() {}
    virtual ~I2SClass() {}

    /* Begin I2S at the given mode / sample rate / bit depth. */
    bool begin(I2SMode mode, uint32_t rate, uint8_t bitsPerSample)
    {
        (void)mode; (void)rate; (void)bitsPerSample;
        _running = true;
        return true;
    }

    void end()
    {
        _running = false;
    }

    /* Read one mono sample (0 = silence). */
    int read()
    {
        (void)_running;
        return 0;
    }

    /* Write one mono sample. */
    bool write(int32_t sample)
    {
        (void)sample;
        return _running;
    }

    /* Write a stereo pair. */
    bool write(int32_t left, int32_t right)
    {
        (void)left; (void)right;
        return _running;
    }

    /* Write a block of mono samples. */
    bool write(const int32_t *samples, size_t count)
    {
        (void)samples; (void)count;
        return _running;
    }

    /* Number of samples currently buffered for reading (0 here). */
    int available()
    {
        return 0;
    }

    bool setRate(uint32_t rate) { (void)rate; return true; }
    void setBitsPerSample(uint8_t bits) { (void)bits; }
    void setMode(I2SMode mode) { (void)mode; }

private:
    bool _running = false;
};

extern I2SClass I2S;

/* ---- legacy core-level I2S C API (I2SInput / I2STransmit examples) ----
 * Older sketches call the ESP8266 core's C functions directly instead of the
 * I2SClass. Same stub behaviour: begin() accepts, read() returns silence. */
#ifdef __cplusplus
extern "C" {
#endif

bool  i2s_rxtx_begin(bool enableRX, bool enableTX);
void  i2s_set_rate(uint32_t rate);
void  i2s_set_clock(uint32_t divider);
bool  i2s_read_sample(int16_t *left, int16_t *right, bool blocking);
bool  i2s_write_sample(uint32_t sample);       /* 32-bit, channels in upper/lower 16 */
bool  i2s_write_sample_nb(uint32_t sample);
bool  i2s_write_lr(int16_t left, int16_t right);

#ifdef __cplusplus
}
#endif

#endif // I2S_H
