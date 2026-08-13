/*
 * HardwareSerial.h — Arduino Serial/Serial1 on the BL602 UARTs.
 *
 * Serial  = UART0 (TX=GPIO16 / RX=GPIO7), the SDK's log console — TX works
 *           out of the box; begin() sets the baud and enables RX interrupts.
 * Serial1 = UART1, default TX=GPIO11 / RX=GPIO17 (see pins_arduino.h); any
 *           UART1-muxable pins can be given to begin(baud, tx, rx).
 *
 * The constructor only sets the UART id; BL602's start.S runs __init_array__,
 * so globals like "Serial1" are properly constructed before main().
 */
#ifndef HardwareSerial_h
#define HardwareSerial_h

#include "Stream.h"
#include <stdint.h>
#include <time.h> /* time_t (ESP8266 detectBaudrate signature) */

#ifndef SERIAL_RX_BUFFER_SIZE
#define SERIAL_RX_BUFFER_SIZE 256
#endif

/* ESP8266 SDK UART FIFO size. SerialDetectBaudrate waits for the TX path to
 * empty by comparing availableForWrite() to this. */
#ifndef UART_TX_FIFO_SIZE
#define UART_TX_FIFO_SIZE 0x80
#endif

/* Data-format / mode enums from the ESP8266 core (cores/esp8266/uart.h).
 * Values keep the ESP8266 bit composition (data bits | parity | stop bits) so
 * sketches that store or compare them behave identically. The BL602 UART is
 * fixed 8N1 full-duplex, so the begin() overloads accept and ignore them. */
enum SerialConfig {
    SERIAL_5N1 = 0x10, SERIAL_6N1 = 0x14, SERIAL_7N1 = 0x18, SERIAL_8N1 = 0x1c,
    SERIAL_5N2 = 0x30, SERIAL_6N2 = 0x34, SERIAL_7N2 = 0x38, SERIAL_8N2 = 0x3c,
    SERIAL_5E1 = 0x12, SERIAL_6E1 = 0x16, SERIAL_7E1 = 0x1a, SERIAL_8E1 = 0x1e,
    SERIAL_5E2 = 0x32, SERIAL_6E2 = 0x36, SERIAL_7E2 = 0x3a, SERIAL_8E2 = 0x3e,
    SERIAL_5O1 = 0x13, SERIAL_6O1 = 0x17, SERIAL_7O1 = 0x1b, SERIAL_8O1 = 0x1f,
    SERIAL_5O2 = 0x33, SERIAL_6O2 = 0x37, SERIAL_7O2 = 0x3b, SERIAL_8O2 = 0x3f,
};

enum SerialMode {
    SERIAL_FULL = 0,
    SERIAL_RX_ONLY = 1,
    SERIAL_TX_ONLY = 2
};

class HardwareSerial : public Stream
{
public:
    HardwareSerial(uint8_t uart_id = 0)
        : _uart_id(uart_id), _rx_overrun(0), _rxbuf_size(SERIAL_RX_BUFFER_SIZE), _baud(0) {}

    void begin(uint32_t baud);                                  /* default pins */
    void begin(uint32_t baud, uint8_t tx_pin, uint8_t rx_pin);  /* remap pins   */

    /* ESP8266-core begin() overloads. config/mode/invert are ignored (BL602
     * UART is fixed 8N1 full-duplex); tx_pin==0 keeps the default pins.
     * SoftwareSerial loopback/repeater and other sketches call these. */
    void begin(uint32_t baud, SerialConfig config) {
        begin(baud, config, SERIAL_FULL, 0, false);
    }
    void begin(uint32_t baud, SerialConfig config, SerialMode mode) {
        begin(baud, config, mode, 0, false);
    }
    void begin(uint32_t baud, SerialConfig config, SerialMode mode, uint8_t tx_pin) {
        begin(baud, config, mode, tx_pin, false);
    }
    void begin(uint32_t baud, SerialConfig config, SerialMode mode, uint8_t tx_pin, bool invert);

    void end();

    int available(void); /* bytes waiting in the RX ring buffer */
    int peek(void);      /* next byte without consuming it, or -1 */
    int read(void);      /* next byte, or -1 if none */

    void flush(void);    /* wait until all TX bytes have been sent */

    size_t write(uint8_t c);
    size_t write(const uint8_t *buf, size_t size);

    /* Import the rest of Print's write() overloads (write(const char*, size)
     * and write(0)) — declaring our own write() above would otherwise hide
     * them, exactly as the ESP8266 core does with `using Print::write;`. */
    using Print::write;

    /* TX is FIFO-empty (blocking writes), so report the full FIFO free —
     * matches ESP8266's uart_tx_free() and lets "wait for printf to drain"
     * loops terminate. */
    int availableForWrite(void) override { return (int)UART_TX_FIFO_SIZE; }

    operator bool() { return _started; }

    /* ESP8266-core RX diagnostics (used by SoftwareSerial loopback/repeater
     * and SerialStress examples). */
    bool hasOverrun(void);          /* true since last check; clears the flag */
    size_t getRxBufferSize(void) { return _rxbuf_size; }

    /* ESP8266-core SerialStress API. BL602 exposes no UART loopback bit and no
     * runtime buffer resize, so swap()/hasRxError() are no-ops and
     * setRxBufferSize() records the requested size while the ISR ring stays at
     * SERIAL_RX_BUFFER_SIZE. Returns the real ring size, like ESP8266 (which
     * returns the size after resize). baudRate() returns the last begin() baud. */
    void swap(void);                       /* remap UART0 pins (no-op) */
    size_t setRxBufferSize(size_t size);   /* recorded; returns SERIAL_RX_BUFFER_SIZE */
    int baudRate(void) { return (int)_baud; }
    bool hasRxError(void) { return false; } /* BL602 ISR tracks only overrun */

    /* ESP8266-core: forward SDK debug output to this port (no-op on BL602,
     * whose console is already on UART0), and auto-baud detection that never
     * blocks (returns the configured baud). */
    void setDebugOutput(bool en);
    unsigned long detectBaudrate(time_t timeoutMillis);

private:
    uint8_t _uart_id;               /* BL602 UART id (0 = console UART) */
    volatile uint8_t _rxbuf[SERIAL_RX_BUFFER_SIZE]; /* written by ISR */
    volatile uint16_t _rx_head;     /* next free slot (ISR side)   */
    volatile uint16_t _rx_tail;     /* next byte to consume (task) */
    volatile uint8_t _rx_overrun;   /* set by ISR when a byte was dropped */
    uint8_t _started;
    size_t _rxbuf_size;             /* reported by getRxBufferSize() */
    uint32_t _baud;                 /* last begin() baud (baudRate()) */

    friend void uart_rx_notify(void *arg); /* ISR-side notifier */
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;

#endif /* HardwareSerial_h */
