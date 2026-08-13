/*
 * SPI.h — SPI master for the Ai-WB2-12F (BL602).
 *
 * Arduino SPI API on top of the SDK's hosal_spi_*. Default pins come from
 * pins_arduino.h (SCK=3 / MOSI=12 / MISO=17 / CS=4). The chip select is
 * software-controlled via hosal_spi_set_cs(). Only one SPI controller exists.
 *
 * polar_phase maps 1:1 to the SPI data modes (0→MODE0 ... 3→MODE3).
 * LSBFIRST is emulated in software (the HOSAL layer is MSB-first).
 */
#ifndef SPI_h
#define SPI_h

#include "Arduino.h" /* BitOrder / LSBFIRST / MSBFIRST */

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

/* classic Arduino SPI clock-divider / speed macros (used by SD/SdFat-style
 * sketches, e.g. Sd2Card.init(SPI_FULL_SPEED, cs)). No-op rate ids here. */
#define SPI_CLOCK_DIV4    0x00
#define SPI_CLOCK_DIV16   0x01
#define SPI_CLOCK_DIV64   0x02
#define SPI_CLOCK_DIV128  0x03
#define SPI_CLOCK_DIV2    0x04
#define SPI_CLOCK_DIV8    0x05
#define SPI_CLOCK_DIV32   0x06
#define SPI_FULL_SPEED    0
#define SPI_HALF_SPEED    1
#define SPI_QUARTER_SPEED 2
#define SPI_EIGHTH_SPEED  3

class SPISettings
{
public:
    SPISettings() : _clk(1000000), _bitOrder(MSBFIRST), _dataMode(SPI_MODE0) {}
    /* bitOrder as uint8_t, matching the ESP8266 core: Adafruit_BusIO passes its
     * own _BitOrder/BusIOBitOrder enum here, which converts to uint8_t but NOT
     * to our BitOrder enum (C++ forbids implicit int<->enum). */
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
        : _clk(clock), _bitOrder((BitOrder)bitOrder), _dataMode(dataMode) {}

    uint32_t clockFreq() const { return _clk; }
    BitOrder bitOrder() const  { return _bitOrder; }
    uint8_t  dataMode() const  { return _dataMode; }

private:
    uint32_t _clk;
    BitOrder _bitOrder;
    uint8_t  _dataMode;
};

class SPIClass
{
public:
    SPIClass();

    void begin(void);                       /* defaults from pins_arduino.h */
    void begin(uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t ss);
    bool pins(int8_t sck, int8_t miso, int8_t mosi, int8_t ss); /* ESP8266 API */
    void end(void);

    void beginTransaction(SPISettings settings); /* asserts CS */
    void endTransaction(void);                   /* deasserts CS */

    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    void transfer(uint8_t *buf, size_t count);
    void transfer(void *buf, uint16_t count);     /* ESP8266 API (void*) */
    void transferBytes(const uint8_t *out, uint8_t *in, uint32_t size); /* ESP8266 full-duplex */
    void write(uint8_t data);                     /* ESP8266 API */
    void write(const uint8_t *buf, size_t count); /* one-way, MISO never read */
    void writeBytes(const uint8_t *data, uint32_t size);   /* ESP8266 API */
    void writePattern(const uint8_t *data, uint8_t size, uint32_t repeat); /* ESP8266 API */
    void write16(uint16_t data);                  /* ESP8266 API */
    void write16(uint16_t data, bool msb);        /* ESP8266 API */
    void write32(uint32_t data);                  /* ESP8266 API */
    void write32(uint32_t data, bool msb);        /* ESP8266 API */

    void setFrequency(uint32_t freq);             /* ESP8266 API */
    void setClock(uint32_t clock) { setFrequency(clock); } /* ESP8266 API */
    void setHwCs(bool use) { (void)use; }         /* ESP8266 API; BL602 CS is always software */
    void setBitOrder(uint8_t order);
    void setDataMode(uint8_t mode);
    void setClockDivider(uint32_t divider); /* base clock 80 MHz */

    void usingInterrupt(uint8_t interruptNumber) { (void)interruptNumber; }

    /* diagnostics: number of transfers that hit the poll timeout */
    uint8_t getFaults(void) const { return _faults; }
    void clearFaults(void) { _faults = 0; }

private:
    uint8_t  _sck;      /* GPIO numbers */
    uint8_t  _mosi;
    uint8_t  _miso;
    uint8_t  _ss;
    uint32_t _freq;
    BitOrder _bitOrder;
    uint8_t  _dataMode;
    uint8_t  _started;
    uint8_t  _inTransaction;
    uint8_t  _faults;

    void _reinit(void);
    void _cs(uint8_t value);
    uint8_t _swap_bits(uint8_t b);
};

extern SPIClass SPI;

#endif /* SPI_h */
