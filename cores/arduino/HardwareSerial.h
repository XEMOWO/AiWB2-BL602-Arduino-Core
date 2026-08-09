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

#ifndef SERIAL_RX_BUFFER_SIZE
#define SERIAL_RX_BUFFER_SIZE 256
#endif

class HardwareSerial : public Stream
{
public:
    HardwareSerial(uint8_t uart_id = 0) : _uart_id(uart_id) {}

    void begin(uint32_t baud);                                  /* default pins */
    void begin(uint32_t baud, uint8_t tx_pin, uint8_t rx_pin);  /* remap pins   */
    void end();

    int available(void); /* bytes waiting in the RX ring buffer */
    int peek(void);      /* next byte without consuming it, or -1 */
    int read(void);      /* next byte, or -1 if none */

    void flush(void);    /* wait until all TX bytes have been sent */

    size_t write(uint8_t c);
    size_t write(const uint8_t *buf, size_t size);

    operator bool() { return _started; }

private:
    uint8_t _uart_id;               /* BL602 UART id (0 = console UART) */
    volatile uint8_t _rxbuf[SERIAL_RX_BUFFER_SIZE]; /* written by ISR */
    volatile uint16_t _rx_head;     /* next free slot (ISR side)   */
    volatile uint16_t _rx_tail;     /* next byte to consume (task) */
    uint8_t _started;

    friend void uart_rx_notify(void *arg); /* ISR-side notifier */
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;

#endif /* HardwareSerial_h */
