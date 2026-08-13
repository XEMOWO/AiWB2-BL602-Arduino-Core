/*
 * wiring_analog.c — analogWrite (PWM) / analogRead (SAR ADC).
 *
 * analogWrite maps the Arduino 0..255 range onto BL602 PWM duty.
 *   - PWM channel = GPIO % 5 (channels are shared across pins, see the board
 *     comment in pins_arduino.h). Calling analogWrite() on a pin that shares
 *     a channel with an active pin steals the channel.
 *   - value 0 stops the PWM. 1..255 → duty (0..100 %).
 *
 * analogRead reads the SAR ADC through the HOSAL layer. Only the 5 broken-out
 * ADC GPIOs (A0..A4, see pins_arduino.h) return data; any other pin returns 0.
 * The ADC is lazily initialized on the first read and channels are added on
 * demand. hosal_adc_value_get() returns millivolts (3.2 V full scale); the
 * result is scaled to 0..4095 (ESP32-style 12-bit resolution).
 *
 * All SDK headers here are C-safe (this is a .c file).
 */
#include "Arduino.h"

#include <bl_pwm.h>
#include <bl_adc.h>
#include <hosal_adc.h>

/* ---- analogWrite: PWM ------------------------------------------------ */

#define PWM_FREQ_DEFAULT 5000 /* Hz, within bl_pwm_init's 2k..800k range */
#define PWM_FREQ_MIN     2000
#define PWM_FREQ_MAX     800000

static uint32_t pwm_freq = PWM_FREQ_DEFAULT;

/* Full-scale value for analogWrite(). ESP8266 exposes analogWriteRange() to
 * change it; default 255 keeps the classic 0..255 mapping. */
static uint32_t pwm_range = 255;

/* Which Arduino pin currently owns each PWM channel (0xFF = free), and the
 * last analogWrite value so analogWriteFrequency() can re-apply it. */
static uint8_t pwm_channel_pin[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t pwm_channel_val[5];

/* Map an analogWrite value (0..pwm_range) to a BL602 PWM duty percentage. */
static float pwm_value_to_duty(uint32_t value)
{
    return (value >= pwm_range) ? 100.0f
                                : (value * 100.0f) / (float)pwm_range;
}

/* ESP8266-core compatibility: on BL602 every GPIO can drive PWM without any
 * prior call (see analogWrite), so nothing needs to be enabled. */
void enablePhaseLockedWaveform(void)
{
}

void analogWrite(uint8_t pin, int value)
{
    uint8_t gpio, ch;

    if (pin >= NUM_DIGITAL_PINS) {
        return;
    }
    gpio = digital_pin_to_gpio[pin];
    ch = gpio % 5;

    if (value <= 0) {
        if (pwm_channel_pin[ch] == pin) {
            bl_pwm_stop(ch);
            pwm_channel_pin[ch] = 0xFF;
            pwm_channel_val[ch] = 0;
        }
        return;
    }

    if (pwm_channel_pin[ch] != pin && pwm_channel_pin[ch] != 0xFF) {
        bl_pwm_stop(ch); /* steal the channel from the previous pin */
    }
    if (pwm_channel_pin[ch] != pin) {
        bl_pwm_init(ch, gpio, pwm_freq);
        pwm_channel_pin[ch] = pin;
    }

    float duty = pwm_value_to_duty((uint32_t)value);
    bl_pwm_set_duty(ch, duty);
    bl_pwm_start(ch);
    pwm_channel_val[ch] = (uint8_t)value;
}

void analogWriteRange(uint32_t range)
{
    if (range == 0) {
        range = 1;
    }
    pwm_range = range;

    /* re-apply the current values at the new full scale */
    for (uint8_t ch = 0; ch < 5; ch++) {
        uint8_t pin = pwm_channel_pin[ch];
        if (pin == 0xFF || pin >= NUM_DIGITAL_PINS) {
            continue;
        }
        float duty = pwm_value_to_duty(pwm_channel_val[ch]);
        bl_pwm_set_duty(ch, duty);
    }
}

void analogWriteFreq(uint32_t freq)
{
    analogWriteFrequency(freq);
}

void analogWriteFrequency(uint32_t freq)
{
    if (freq < PWM_FREQ_MIN) {
        freq = PWM_FREQ_MIN;
    } else if (freq > PWM_FREQ_MAX) {
        freq = PWM_FREQ_MAX;
    }
    pwm_freq = freq;

    /* re-arm every active channel at the new frequency */
    for (uint8_t ch = 0; ch < 5; ch++) {
        uint8_t pin = pwm_channel_pin[ch];
        if (pin == 0xFF || pin >= NUM_DIGITAL_PINS) {
            continue;
        }
        uint8_t gpio = digital_pin_to_gpio[pin];
        bl_pwm_init(ch, gpio, pwm_freq);
        float duty = pwm_value_to_duty(pwm_channel_val[ch]);
        bl_pwm_set_duty(ch, duty);
        bl_pwm_start(ch);
    }
}

/* ---- analogRead: SAR ADC ---------------------------------------------- */

static hosal_adc_dev_t adc_dev;
static uint8_t adc_inited = 0;
static uint8_t adc_chan_added[ADC_CHANNEL_MAX];

/* Resolve an Arduino pin (A0..A4 or raw GPIO number) to an ADC channel,
 * or -1 if the pin is not ADC-capable. */
static int pin_to_adc_channel(uint8_t pin)
{
    uint8_t gpio;
    int ch;

    if (pin >= NUM_DIGITAL_PINS && pin < NUM_DIGITAL_PINS + NUM_ANALOG_INPUTS) {
        gpio = analog_pin_to_gpio[pin - NUM_DIGITAL_PINS];
    } else if (pin < NUM_DIGITAL_PINS) {
        gpio = digital_pin_to_gpio[pin];
    } else {
        return -1;
    }

    ch = bl_adc_get_channel_by_gpio(gpio);
    return (ch >= 0) ? ch : -1;
}

/* Read one channel in millivolts (lazy-init on first use). */
static int adc_read_mv(uint8_t pin)
{
    int ch = pin_to_adc_channel(pin);
    int mv;

    if (ch < 0) {
        return 0;
    }

    /* Lazy one-shot init on the first ADC-capable read. config.pin must be a
     * valid ADC GPIO — it is, by construction of pin_to_adc_channel(). */
    if (!adc_inited) {
        uint8_t gpio = (pin < NUM_DIGITAL_PINS)
            ? digital_pin_to_gpio[pin]
            : analog_pin_to_gpio[pin - NUM_DIGITAL_PINS];

        memset(&adc_dev, 0, sizeof adc_dev);
        adc_dev.port = 0;
        adc_dev.config.mode = HOSAL_ADC_ONE_SHOT;
        adc_dev.config.pin = gpio;
        adc_dev.config.sampling_freq = 340; /* max for one-shot mode */
        if (hosal_adc_init(&adc_dev) != 0) {
            return 0;
        }
        adc_inited = 1;
    }

    if (!adc_chan_added[ch]) {
        /* add_channel() only re-muxes the init pin, so mux this GPIO to the
         * ANALOG function explicitly before adding the channel. */
        uint8_t gpio = (pin < NUM_DIGITAL_PINS)
            ? digital_pin_to_gpio[pin]
            : analog_pin_to_gpio[pin - NUM_DIGITAL_PINS];
        bl_adc_gpio_init(gpio);
        if (hosal_adc_add_channel(&adc_dev, ch) != 0) {
            return 0;
        }
        adc_chan_added[ch] = 1;
    }

    mv = hosal_adc_value_get(&adc_dev, ch, 1000); /* millivolts */
    return (mv < 0) ? 0 : mv;
}

/* Configured analogRead resolution in bits (default 12, ESP32-style). */
static uint8_t adc_bits = 12;

void analogReadResolution(uint8_t bits)
{
    if (bits < 8) {
        bits = 8;
    } else if (bits > 16) {
        bits = 16;
    }
    adc_bits = bits;
}

int analogRead(uint8_t pin)
{
    uint32_t full = (1UL << adc_bits) - 1;
    return (int)(((uint32_t)adc_read_mv(pin) * full + 1600) / 3200); /* @3.2V */
}

int analogReadMilliVolts(uint8_t pin)
{
    return adc_read_mv(pin);
}
