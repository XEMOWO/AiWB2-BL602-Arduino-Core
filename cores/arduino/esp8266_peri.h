/*
 * esp8266_peri.h — ESP8266-core peripheral-register header (BL602 shim).
 *
 * The Xtensa ESP8266 exposes its memory-mapped peripherals through this
 * header (ETS_UART_INTR_*, PIN_FUNC_SELECT, register macros). Some third-party
 * sketches include it out of habit without touching any register. There is no
 * equivalent register map on BL602, so this file provides only the include
 * guard plus the few generic helpers that are safe on any architecture.
 */
#ifndef ESP8266_PERI_H
#define ESP8266_PERI_H

/* ESP8266 watchdog helper macros; BL602 versions live in Esp.h. */
#include <stdint.h>

/* GPIO_REG / GPIO_INPUT_VAL / GPIO_OUTPUT_VAL — the real BL602 pad-level and
 * output-drive registers (see wiring_private.h). */
#include "wiring_private.h"

/* Memory-barrier helper used by some peripherals (maps to a compiler fence —
 * correct on RISC-V, which is strongly ordered at the MMIO level we use). */
#define PORT_ENTER_CRITICAL()  noInterrupts()
#define PORT_EXIT_CRITICAL()   interrupts()

/* ---- GPIO input register readback (interactive.ino) ----
 * The ESP8266's GPI is the GPIO_IN status register; `interactive` probes the
 * boot-strap pins as `(GPI >> 16) & 0xf` to decide between soft/hard reset.
 * BL602 boots the same way every time, so a constant keeps the branch a
 * "must hard reset" no-op while letting the sketch compile unchanged. */
#define GPI 0

/* ---- GPIO output / GPIO16 input readback (Graph.ino) ----
 * Graph.ino builds `((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)` as a JSON
 * value. On BL602 the GPIO output-drive register is readable, so GPO reports
 * the live output levels; GP16I mirrors the ESP8266 convention (bit 0 =
 * GPIO16 input level). GPI stays 0 (see above). */
#define GPO   GPIO_REG(GPIO_OUTPUT_VAL)           /* bit = pin, output drive */
#define GP16I (GPIO_REG(GPIO_INPUT_VAL) >> 16)    /* bit 0 = GPIO16 input   */

/* ---- ESP8266_DREG(addr) (MMU48K etc.) ----
 * On Xtensa this dereferences the peripheral register window 0x3FF00000+addr.
 * That window does not exist on BL602 — dereferencing it would trap. Map it to
 * a dead volatile word instead, so `ESP8266_DREG(0x24)` (the iRAM-bank
 * readback MMU48K prints) compiles and returns 0. Backing in esp_compat.c. */
extern volatile uint32_t wb2_dreg_shim[256];
#define ESP8266_DREG(addr)  (wb2_dreg_shim[((addr) & 0x3FF) >> 2])

/* ---- UART config register (SerialStress.ino) ----
 * `USC0(0) |= (1 << UCLBE)` enables the UART internal loopback on ESP8266.
 * BL602 has no such loopback bit; back it with a real writable variable so the
 * expression compiles and is harmless. wb2_usart_conf is defined in
 * esp_compat.c. */
extern volatile uint32_t wb2_usart_conf[2];
#define USC0(u)  (wb2_usart_conf[(u) & 1])
#define UCLBE    14 /* loop-back enable bit position (kept for source compat) */

/* ---- SPI1 legacy slave registers (SPISlave library) ----
 * hspi_slave.c pokes the Xtensa SPI1 peripheral through esp8266_peri.h
 * (SPI1CMD/SPI1S/SPI1W(p)/... and the SPISxx bit masks). BL602's SPI
 * controller has no such register window, so every name maps to a dead
 * volatile word: reads return whatever was last written (0 by default, so the
 * ISR sees no pending status and never fires) and writes are harmless. The
 * values/bit-positions below are the ESP8266 SDK's originals so the source
 * compiles unchanged. Backing storage is wb2_spi1_reg[] in esp_compat.c. */
#define SPECIAL 0x02 /* ESP8266 special-function pin mode; pinMode() treats
                      * unknown modes as INPUT, so this is safe at runtime. */

extern volatile uint32_t wb2_spi1_reg[32];
#define WB2_SPI1_REG(addr)  (wb2_spi1_reg[((addr) - 0x100) >> 2])

#define SPI1CMD  WB2_SPI1_REG(0x100)
#define SPI1C2   WB2_SPI1_REG(0x114)
#define SPI1CLK  WB2_SPI1_REG(0x118)
#define SPI1U    WB2_SPI1_REG(0x11C)
#define SPI1U1   WB2_SPI1_REG(0x120)
#define SPI1U2   WB2_SPI1_REG(0x124)
#define SPI1WS   WB2_SPI1_REG(0x128)
#define SPI1P    WB2_SPI1_REG(0x12C)
#define SPI1S    WB2_SPI1_REG(0x130)
#define SPI1S1   WB2_SPI1_REG(0x134)
/* 16 write-buffer words, one per word-address, as on the ESP8266. */
#define SPI1W(p) WB2_SPI1_REG(0x140 + (((p) & 0xF) * 4))
extern volatile uint32_t wb2_spi1_status; /* SPIIR readback */
#define SPIIR    wb2_spi1_status
/* SPI0 interrupt-clear reg (hspi_slave ISR clears it on SPI0 IRQ). */
#define SPI0S    wb2_spi1_reg[1]

#define SPII0     4 /* SPI0 Interrupt */
#define SPII1     7 /* SPI1 Interrupt */
#define SPII2     9 /* I2S Interrupt */

#define SPIBUSY    (1 << 18) /* SPI_USR */
#define SPIC2MOSIDN_S 23     /* SPI_MOSI_DELAY_NUM_S */
#define SPIC2MISODM_S 16     /* SPI_MISO_DELAY_MODE_S */
#define SPIUCOMMAND (1 << 31) /* COMMAND phase, SPI_USR_COMMAND */
#define SPIUMISOH   (1 << 24) /* MISO phase uses W8-W15, SPI_USR_DIN_HIGHPART */
#define SPIUSSE     (1 << 6)  /* SPI Slave Edge, SPI_CK_I_EDGE */
#define SPILCOMMAND 28        /* 4 bit in SPIxU2 (7=8bit) */
#define SPISSRES    (1 << 31) /* SYNC RESET, SPI_SYNC_RESET */
#define SPISE       (1 << 30) /* Slave Enable, SPI_SLAVE_MODE */
#define SPISBE      (1 << 29) /* WR/RD BUF enable, SPI_SLV_WR_RD_BUF_EN */
#define SPISTRIE    (1 << 9)  /* TRANS interrupt enable */
#define SPISWSIE    (1 << 8)  /* WR_STA interrupt enable */
#define SPISRSIE    (1 << 7)  /* RD_STA interrupt enable */
#define SPISWBIE    (1 << 6)  /* WR_BUF interrupt enable */
#define SPISRBIE    (1 << 5)  /* RD_BUF interrupt enable */
#define SPISWSIS    (1 << 3)  /* WR_STA interrupt status */
#define SPISRSIS    (1 << 2)  /* RD_STA interrupt status */
#define SPISWBIS    (1 << 1)  /* WR_BUF interrupt status */
#define SPISRBIS    (1 << 0)  /* RD_BUF interrupt status */
#define SPIS1LSTA   27        /* 5 bit in SPIxS1, SPI_SLV_STATUS_BITLEN */
#define SPIS1RSTA   (1 << 25) /* enable STA read from Master, SPI_SLV_STATUS_READBACK */
#define SPIS1LBUF   16        /* 9 bit in SPIxS1, SPI_SLV_BUF_BITLEN */
#define SPIS1LRBA   10        /* 6 bit in SPIxS1, SPI_SLV_RD_ADDR_BITLEN */
#define SPIS1LWBA   4         /* 6 bit in SPIxS1, SPI_SLV_WR_ADDR_BITLEN */

#endif /* ESP8266_PERI_H */
