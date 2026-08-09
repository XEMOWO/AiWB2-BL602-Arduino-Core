/*
 * Wire.cpp — TwoWire (I2C master) on hosal_i2c.
 *
 * TX path: beginTransmission()/write() queue bytes; endTransmission() issues
 * the whole write with one hosal_i2c_master_send().
 * RX path: requestFrom() issues one hosal_i2c_master_recv() into the ring
 * buffer; available()/read()/peek() consume it.
 *
 * hosal_i2c.h carries extern "C" guards, so it is included directly.
 */
#include "Wire.h"

#include <string.h>
#include <Arduino.h>
#include <hosal_i2c.h>

/* One shared HOSAL device (BL602 has a single I2C controller). */
static hosal_i2c_dev_t s_dev;

TwoWire Wire;

TwoWire::TwoWire()
    : _sda(PIN_WIRE_SDA),
      _scl(PIN_WIRE_SCL),
      _freq(100000),
      _addr(0),
      _txlen(0),
      _txoverflow(0),
      _rxhead(0),
      _rxtail(0),
      _started(0)
{
}

void TwoWire::begin(void)
{
    begin(_sda, _scl);
}

void TwoWire::begin(uint8_t sda, uint8_t scl)
{
    uint8_t sda_gpio = (sda < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[sda] : sda;
    uint8_t scl_gpio = (scl < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[scl] : scl;

    if (_started) {
        hosal_i2c_finalize(&s_dev);
    }

    _sda = sda_gpio;
    _scl = scl_gpio;

    memset(&s_dev, 0, sizeof(s_dev));
    s_dev.port = 0;
    s_dev.config.address_width = HOSAL_I2C_ADDRESS_WIDTH_7BIT;
    s_dev.config.freq = _freq;
    s_dev.config.mode = HOSAL_I2C_MODE_MASTER;
    s_dev.config.scl = _scl;
    s_dev.config.sda = _sda;

    hosal_i2c_init(&s_dev);
    _started = 1;
}

void TwoWire::end(void)
{
    if (_started) {
        hosal_i2c_finalize(&s_dev);
        _started = 0;
    }
}

void TwoWire::setClock(uint32_t freq)
{
    _freq = freq;
    if (_started) {
        begin(_sda, _scl); /* re-init with the new clock */
    }
}

void TwoWire::beginTransmission(uint8_t address)
{
    _addr = address;
    _txlen = 0;
    _txoverflow = 0;
}

size_t TwoWire::write(uint8_t data)
{
    if (_txlen >= I2C_TX_BUFFER_SIZE) {
        _txoverflow = 1;
        return 0;
    }
    _txbuf[_txlen++] = data;
    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
    size_t n = 0;
    for (size_t i = 0; i < quantity; i++) {
        if (write(data[i])) {
            n++;
        }
    }
    return n;
}

uint8_t TwoWire::endTransmission(bool stopBit)
{
    (void)stopBit; /* single-shot master sends, always with a stop */
    int res = 0;

    if (_txoverflow) {
        res = 1; /* buffer overflowed before the send */
    } else if (hosal_i2c_master_send(&s_dev, _addr, _txbuf, _txlen, 1000) != 0) {
        res = 2; /* NACK / I/O error */
    }
    _txlen = 0;
    _txoverflow = 0;
    return res;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
    if (quantity > I2C_RX_BUFFER_SIZE - 1) {
        quantity = I2C_RX_BUFFER_SIZE - 1; /* keep the ring mask valid */
    }
    _rxhead = _rxtail = 0;
    if (quantity == 0) {
        return 0;
    }
    if (hosal_i2c_master_recv(&s_dev, address, _rxbuf, quantity, 1000) != 0) {
        return 0;
    }
    _rxhead = quantity;
    return quantity;
}

int TwoWire::available(void)
{
    return (int)((_rxhead - _rxtail) & (I2C_RX_BUFFER_SIZE - 1));
}

int TwoWire::read(void)
{
    if (_rxhead == _rxtail) {
        return -1;
    }
    int c = _rxbuf[_rxtail];
    _rxtail = (uint16_t)((_rxtail + 1) & (I2C_RX_BUFFER_SIZE - 1));
    return c;
}

int TwoWire::peek(void)
{
    if (_rxhead == _rxtail) {
        return -1;
    }
    return _rxbuf[_rxtail];
}
