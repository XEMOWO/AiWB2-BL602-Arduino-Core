/*
 * SPI.cpp — SPI master for the Ai-WB2-12F (BL602), polled, no interrupts.
 *
 * Implementation note: the SDK's hosal_spi_* send path is interrupt driven
 * (SPI_IRQn -> spi_irq_process -> xEventGroupSetBitsFromISR). On this core's
 * ROM-driver build that IRQ never fires, so every transfer dead-locked inside
 * xEventGroupWaitBits(). We therefore configure the controller with the
 * bl602_std drivers but transfer with the pure-register polled functions
 * SPI_Send_8bits() / SPI_SendRecv_8bits() — no IRQ, no event group, and a
 * bounded timeout so a faulting link returns instead of hanging.
 *
 * The CS pin is software-controlled (bl_gpio). polar_phase maps 1:1 to the
 * SPI data modes (0->MODE0 ... 3->MODE3); LSBFIRST is emulated in software.
 */
#include "SPI.h"

#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

extern "C" {
#include <bl602.h>         /* SPI_ID_0, SPI_BASE, register macros          */
#include <bl602_spi.h>     /* SPI_Init, SPI_Send_8bits, ...                */
#include <bl602_glb.h>     /* GLB_AHB_Slave1_Reset, SPI pad mode           */
#include <bl602_gpio.h>    /* GLB_GPIO_Func_Init, GPIO_FUN_SPI, pin type   */
#include <bl_gpio.h>       /* bl_gpio_enable_output / bl_gpio_output_set   */
}

/* One shared SPI controller (BL602 has a single SPI0). */
static SPI_ID_Type const s_port = SPI_ID_0;

SPIClass SPI;

SPIClass::SPIClass()
    : _sck(PIN_SPI_SCK),
      _mosi(PIN_SPI_MOSI),
      _miso(PIN_SPI_MISO),
      _ss(PIN_SPI_SS),
      _freq(1000000),
      _bitOrder(MSBFIRST),
      _dataMode(SPI_MODE0),
      _started(0),
      _inTransaction(0),
      _faults(0)
{
}

void SPIClass::begin(void)
{
    begin(_sck, _mosi, _miso, _ss);
}

void SPIClass::begin(uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t ss)
{
    _sck = (sck  < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[sck]  : sck;
    _mosi = (mosi < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[mosi] : mosi;
    _miso = (miso < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[miso] : miso;
    _ss = (ss    < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[ss]    : ss;

    _reinit();
    _cs(HIGH); /* CS idle-high */
    _started = 1;
}

void SPIClass::_reinit(void)
{
    SPI_CFG_Type spicfg;
    SPI_FifoCfg_Type fifocfg;
    GLB_GPIO_Type pins[3];

    if (_started) {
        SPI_Disable(s_port, SPI_WORK_MODE_MASTER);
    }

    /* Pin mux: SCK + MOSI + MISO -> SPI0 function.
     *
     * NOTE: hosal_spi's original pin list forced GPIO22 in as well, but on the
     * WB2-12F GPIO 20/21/22 are shared with the internal flash. Re-muxing 22 to
     * SPI on this XIP build froze the CPU mid-instruction-fetch (no print, no
     * panic — exactly what we saw). Only the three bus pins are touched here. */
    pins[0] = (GLB_GPIO_Type)_sck;
    pins[1] = (GLB_GPIO_Type)_mosi;
    pins[2] = (GLB_GPIO_Type)_miso;
    GLB_GPIO_Func_Init(GPIO_FUN_SPI, pins, 3);
    GLB_Set_SPI_0_ACT_MOD_Sel(GLB_SPI_PAD_ACT_AS_MASTER);

    GLB_AHB_Slave1_Reset(BL_AHB_SLAVE1_SPI);
    SPI_SetClock(s_port, _freq);
    SPI_SetDeglitchCount(s_port, 0x2);

    memset(&spicfg, 0, sizeof(spicfg));
    spicfg.deglitchEnable = DISABLE;
    spicfg.continuousEnable = ENABLE;
    spicfg.byteSequence = SPI_BYTE_INVERSE_BYTE0_FIRST;
    spicfg.bitSequence = SPI_BIT_INVERSE_MSB_FIRST;
    spicfg.frameSize = SPI_FRAME_SIZE_32;
    switch (_dataMode) {
    case SPI_MODE1:
        spicfg.clkPhaseInv = SPI_CLK_PHASE_INVERSE_1;
        spicfg.clkPolarity = SPI_CLK_POLARITY_LOW;
        break;
    case SPI_MODE2:
        spicfg.clkPhaseInv = SPI_CLK_PHASE_INVERSE_0;
        spicfg.clkPolarity = SPI_CLK_POLARITY_HIGH;
        break;
    case SPI_MODE3:
        spicfg.clkPhaseInv = SPI_CLK_PHASE_INVERSE_1;
        spicfg.clkPolarity = SPI_CLK_POLARITY_HIGH;
        break;
    default: /* SPI_MODE0 */
        spicfg.clkPhaseInv = SPI_CLK_PHASE_INVERSE_0;
        spicfg.clkPolarity = SPI_CLK_POLARITY_LOW;
        break;
    }
    SPI_Init(s_port, &spicfg);
    SPI_Disable(s_port, SPI_WORK_MODE_MASTER);

    /* Polled mode: keep the SPI interrupt masked. */
    SPI_IntMask(s_port, SPI_INT_ALL, MASK);

    fifocfg.txFifoThreshold = 1;
    fifocfg.rxFifoThreshold = 1;
    fifocfg.txFifoDmaEnable = DISABLE;
    fifocfg.rxFifoDmaEnable = DISABLE;
    SPI_FifoConfig(s_port, &fifocfg);

    SPI_Enable(s_port, SPI_WORK_MODE_MASTER);
}

void SPIClass::_cs(uint8_t value)
{
    bl_gpio_enable_output(_ss, 0, 0);
    bl_gpio_output_set(_ss, value);
}

void SPIClass::end(void)
{
    if (_started) {
        _cs(HIGH);
        SPI_Disable(s_port, SPI_WORK_MODE_MASTER);
        _started = 0;
    }
}

uint8_t SPIClass::_swap_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void SPIClass::beginTransaction(SPISettings settings)
{
    uint8_t order = settings.bitOrder();
    uint8_t mode = settings.dataMode();
    uint32_t freq = settings.clockFreq();

    if (_started && (freq != _freq || mode != _dataMode)) {
        _freq = freq;
        _dataMode = mode;
        _reinit();
    } else {
        _freq = freq;
        _dataMode = mode;
    }
    _bitOrder = order;
    _inTransaction = 1;
    _cs(LOW); /* assert CS for the transaction */
}

void SPIClass::endTransaction(void)
{
    _inTransaction = 0;
    _cs(HIGH);
}

uint8_t SPIClass::transfer(uint8_t data)
{
    uint8_t tx = data, rx = 0;
    uint8_t own_cs = 0;

    if (!_inTransaction) {
        _cs(LOW);
        own_cs = 1;
    }
    if (_bitOrder == LSBFIRST) {
        tx = _swap_bits(tx);
    }
    if (SPI_SendRecv_8bits(s_port, &tx, &rx, 1, SPI_TIMEOUT_ENABLE) != SUCCESS) {
        _faults++;
    }
    if (_bitOrder == LSBFIRST) {
        rx = _swap_bits(rx);
    }
    if (own_cs) {
        _cs(HIGH);
    }
    return rx;
}

void SPIClass::transfer(uint8_t *buf, size_t count)
{
    uint8_t *rx;
    uint8_t own_cs = 0;
    size_t i;

    if (count == 0) {
        return;
    }
    rx = (uint8_t *)malloc(count);
    if (!rx) {
        return;
    }

    if (!_inTransaction) {
        _cs(LOW);
        own_cs = 1;
    }
    if (_bitOrder == LSBFIRST) {
        for (i = 0; i < count; i++) {
            buf[i] = _swap_bits(buf[i]);
        }
    }
    if (SPI_SendRecv_8bits(s_port, buf, rx, (uint32_t)count, SPI_TIMEOUT_ENABLE) != SUCCESS) {
        _faults++;
    }
    memcpy(buf, rx, count);
    if (_bitOrder == LSBFIRST) {
        for (i = 0; i < count; i++) {
            buf[i] = _swap_bits(buf[i]);
        }
    }
    if (own_cs) {
        _cs(HIGH);
    }
    free(rx);
}

uint16_t SPIClass::transfer16(uint16_t data)
{
    uint8_t hi = transfer((uint8_t)(data >> 8));
    uint8_t lo = transfer((uint8_t)data);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

void SPIClass::write(const uint8_t *buf, size_t count)
{
    uint8_t own_cs = 0;
    uint8_t *tmp = NULL;
    size_t i;

    if (count == 0) {
        return;
    }
    if (!_inTransaction) {
        _cs(LOW);
        own_cs = 1;
    }
    /* One-way transfer: the slave's MISO (none on ST7789) is never read, so
     * the source buffer is left untouched. */
    if (_bitOrder == LSBFIRST) {
        tmp = (uint8_t *)malloc(count);
        if (!tmp) {
            goto out;
        }
        for (i = 0; i < count; i++) {
            tmp[i] = _swap_bits(buf[i]);
        }
        if (SPI_Send_8bits(s_port, tmp, (uint32_t)count, SPI_TIMEOUT_ENABLE) != SUCCESS) {
            _faults++;
        }
        free(tmp);
    } else {
        if (SPI_Send_8bits(s_port, (uint8_t *)buf, (uint32_t)count, SPI_TIMEOUT_ENABLE) != SUCCESS) {
            _faults++;
        }
    }
out:
    if (own_cs) {
        _cs(HIGH);
    }
}

void SPIClass::setBitOrder(uint8_t order)
{
    _bitOrder = order;
}

void SPIClass::setDataMode(uint8_t mode)
{
    if (mode > SPI_MODE3) {
        mode = SPI_MODE0;
    }
    if (_started && mode != _dataMode) {
        _dataMode = mode;
        _reinit();
    } else {
        _dataMode = mode;
    }
}

void SPIClass::setClockDivider(uint32_t divider)
{
    if (divider == 0) {
        divider = 1;
    }
    _freq = 80000000UL / divider; /* BL602 BCLK = 80 MHz */
    if (_started) {
        _reinit();
    }
}
