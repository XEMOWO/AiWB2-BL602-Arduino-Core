/*
 * Wire.h — TwoWire (I2C master) for the Ai-WB2-12F (BL602).
 *
 * Arduino Wire API on top of the SDK's hosal_i2c_*. Default pins come from
 * pins_arduino.h (SCL=GPIO12 / SDA=GPIO3); pass explicit pins to Wire.begin()
 * to override. Only one I2C master controller exists on BL602.
 */
#ifndef Wire_h
#define Wire_h

#include <stdint.h>
#include <stddef.h>

#include "Print.h"

#define I2C_TX_BUFFER_SIZE 256
#define I2C_RX_BUFFER_SIZE 256 /* power of two (ring index mask) */

class TwoWire : public Print
{
public:
    TwoWire();

    void begin(void);                              /* defaults from pins_arduino.h */
    void begin(uint8_t sda, uint8_t scl);
    /* ESP8266-compatible master address parameter (ignored on master mode,
     * used as slave address on WB2 if the HOSAL driver is in slave mode). */
    void begin(int sda, int scl, uint8_t address) { (void)address; begin((uint8_t)sda, (uint8_t)scl); }
    void begin(uint8_t address);                   /* join bus as slave @ address */
    void end(void);
    void setClock(uint32_t freq);
    /* ESP8266 API: I2C slave clock-stretch budget (µs). The BL602 HOSAL I2C
     * driver always stretches as long as the slave needs, so this is a no-op. */
    void setClockStretchLimit(uint32_t us) { (void)us; }

    /* ---- I2C slave callbacks (ESP8266 Wire API) ---- */
    void onReceive(void (*)(int));                 /* AVR style */
    void onReceive(void (*)(size_t));              /* ESP8266 style */
    void onRequest(void (*)(void));
    static void onReceiveService(uint8_t*, size_t); /* called from the bus driver */
    static void onRequestService(void);

    void beginTransmission(uint8_t address);
    uint8_t endTransmission(bool stopBit = true); /* 0 = ok, 1 = buffer overflow, 2 = I/O error */

    uint8_t requestFrom(uint8_t address, uint8_t quantity);
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop);

    size_t write(uint8_t data) override;          /* queue into TX buffer */
    size_t write(const uint8_t *data, size_t quantity) override;

    /* Re-expose Print's write(const char*) / write(int) overloads, hidden by
     * the write() overrides above (C++ name hiding). Without this,
     * Wire.write("hello") fails to compile. */
    using Print::write;

    int available(void);                          /* bytes pending in RX buffer */
    int read(void);                               /* -1 if empty */
    int peek(void);

    static void (*user_onRequest)(void);           /* slave request-from-master cb */
    static void (*user_onReceive)(size_t);         /* slave receive-from-master cb */

private:
    uint8_t  _sda;        /* GPIO numbers */
    uint8_t  _scl;
    uint32_t _freq;
    uint8_t  _addr;
    uint8_t  _txbuf[I2C_TX_BUFFER_SIZE];
    uint16_t _txlen;
    uint8_t  _txoverflow;
    uint8_t  _rxbuf[I2C_RX_BUFFER_SIZE];
    uint16_t _rxhead;
    uint16_t _rxtail;
    uint8_t  _started;
};

extern TwoWire Wire;

#endif /* Wire_h */
