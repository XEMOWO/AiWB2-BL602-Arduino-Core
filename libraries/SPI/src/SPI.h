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

#include <stdint.h>
#include <stddef.h>

#define MSBFIRST 0
#define LSBFIRST 1

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

class SPISettings
{
public:
    SPISettings() : _clk(1000000), _bitOrder(MSBFIRST), _dataMode(SPI_MODE0) {}
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
        : _clk(clock), _bitOrder(bitOrder), _dataMode(dataMode) {}

    uint32_t clockFreq() const { return _clk; }
    uint8_t  bitOrder() const  { return _bitOrder; }
    uint8_t  dataMode() const  { return _dataMode; }

private:
    uint32_t _clk;
    uint8_t  _bitOrder;
    uint8_t  _dataMode;
};

class SPIClass
{
public:
    SPIClass();

    void begin(void);                       /* defaults from pins_arduino.h */
    void begin(uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t ss);
    void end(void);

    void beginTransaction(SPISettings settings); /* asserts CS */
    void endTransaction(void);                   /* deasserts CS */

    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    void transfer(uint8_t *buf, size_t count);
    void write(const uint8_t *buf, size_t count); /* one-way, MISO never read */

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
    uint8_t  _bitOrder;
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
