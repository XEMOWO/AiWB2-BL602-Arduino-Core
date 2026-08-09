/*
 * HardwareSerial.cpp — Serial (UART0) and Serial1 (UART1).
 *
 * TX: polled writes through bl_uart_data_send().
 * RX: the shared UART IRQ drains both FIFOs into per-port ring buffers;
 *     available()/read() consume from the task side.
 */
#include "HardwareSerial.h"

#include "Arduino.h"

/* bl_uart / bl_irq live in the SDK's hosal layer. bl_uart.h pulls in the
 * chip headers (bl602.h), so UART0_IRQn/UART1_IRQn and UART_* helpers are
 * visible.
 *
 * The SDK's C headers have no extern "C" guards, so including them from C++
 * would mangle every symbol and fail at link — wrap them explicitly. */
extern "C" {
#include <bl_uart.h>
#include <bl_irq.h>
}

HardwareSerial Serial;
HardwareSerial Serial1(1);

/* ISR-side notifier: drain the whole RX FIFO of the object's port into its
 * ring buffer. arg = the object (a HardwareSerial). */
void uart_rx_notify(void *arg)
{
    HardwareSerial *s = (HardwareSerial *)arg;
    int c;

    while ((c = bl_uart_data_recv(s->_uart_id)) >= 0) {
        uint16_t next = (uint16_t)((s->_rx_head + 1) & (SERIAL_RX_BUFFER_SIZE - 1));
        if (next == s->_rx_tail) {
            break; /* ring full: drop the newest byte */
        }
        s->_rxbuf[s->_rx_head] = (uint8_t)c;
        s->_rx_head = next;
    }
}

/* Our own UART ISR. The SDK only defines the ISR under the HAL driver; in
 * this ROM-driver build those symbols do not exist, so we register this one
 * for both UART0_IRQn and UART1_IRQn. RX_END / RTO must be cleared
 * explicitly, or the edge-triggered interrupt would re-fire. */
static void uart_irq_handler(void)
{
    for (uint8_t id = 0; id <= 1; id++) {
        UART_ID_Type u = (UART_ID_Type)id;

        if (UART_GetIntStatus(u, UART_INT_RX_END) == SET ||
            UART_GetIntStatus(u, UART_INT_RX_FIFO_REQ) == SET ||
            UART_GetIntStatus(u, UART_INT_RTO) == SET) {
            uart_rx_notify((id == 0) ? (void *)&Serial : (void *)&Serial1);
            UART_IntClear(u, UART_INT_RX_END);
            UART_IntClear(u, UART_INT_RTO);
            UART_IntClear(u, UART_INT_RX_FER); /* overflow safety */
        }
    }
}

/* Register our ISR for one IRQ (once). Returns 0 when first registered. */
static int ensure_irq(IRQn_Type irq, uint8_t *flag)
{
    if (*flag) {
        return 1;
    }
    bl_irq_register(irq, (void *)uart_irq_handler);
    bl_irq_enable(irq);
    *flag = 1;
    return 0;
}

void HardwareSerial::begin(uint32_t baud)
{
    if (_uart_id == 0) {
        /* UART0 is already up (SDK console); just adopt the baud rate.
         * Note: printf() shares this UART, so it follows the baud too —
         * exactly like Serial on ESP32. */
        bl_uart_setbaud(0, baud);
    } else {
        /* UART1 needs full init (clock + GPIO mux + UART config). */
        bl_uart_init(_uart_id, PIN_SERIAL1_TX, PIN_SERIAL1_RX, 0, 0, baud);
    }

    /* RX: hook the UART interrupt to the ring buffer. */
    bl_uart_int_rx_notify_register(_uart_id, uart_rx_notify, this);
    bl_uart_int_rx_enable(_uart_id);

    static uint8_t irq0_done = 0, irq1_done = 0;
    if (_uart_id == 0) {
        ensure_irq(UART0_IRQn, &irq0_done);
    } else {
        ensure_irq(UART1_IRQn, &irq1_done);
    }

    _started = 1;
}

void HardwareSerial::begin(uint32_t baud, uint8_t tx_pin, uint8_t rx_pin)
{
    uint8_t tx = (tx_pin < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[tx_pin] : tx_pin;
    uint8_t rx = (rx_pin < NUM_DIGITAL_PINS) ? digital_pin_to_gpio[rx_pin] : rx_pin;

    /* bl_uart_init handles clock + GPIO mux + UART config for any port. */
    bl_uart_init(_uart_id, tx, rx, 0, 0, baud);

    bl_uart_int_rx_notify_register(_uart_id, uart_rx_notify, this);
    bl_uart_int_rx_enable(_uart_id);

    static uint8_t irq0_done = 0, irq1_done = 0;
    if (_uart_id == 0) {
        ensure_irq(UART0_IRQn, &irq0_done);
    } else {
        ensure_irq(UART1_IRQn, &irq1_done);
    }

    _started = 1;
}

void HardwareSerial::end(void)
{
    bl_uart_int_rx_disable(_uart_id);
    bl_uart_int_rx_notify_unregister(_uart_id, uart_rx_notify, this);
    _started = 0;
}

int HardwareSerial::available(void)
{
    return (int)((_rx_head - _rx_tail) & (SERIAL_RX_BUFFER_SIZE - 1));
}

int HardwareSerial::peek(void)
{
    if (_rx_head == _rx_tail) {
        return -1;
    }
    return _rxbuf[_rx_tail];
}

int HardwareSerial::read(void)
{
    if (_rx_head == _rx_tail) {
        return -1;
    }
    int c = _rxbuf[_rx_tail];
    _rx_tail = (uint16_t)((_rx_tail + 1) & (SERIAL_RX_BUFFER_SIZE - 1));
    return c;
}

void HardwareSerial::flush(void)
{
    bl_uart_flush(_uart_id);
}

size_t HardwareSerial::write(uint8_t c)
{
    bl_uart_data_send(_uart_id, c);
    return 1;
}

size_t HardwareSerial::write(const uint8_t *buf, size_t size)
{
    size_t n = 0;
    for (size_t i = 0; i < size; i++) {
        bl_uart_data_send(_uart_id, buf[i]);
        n++;
    }
    return n;
}
