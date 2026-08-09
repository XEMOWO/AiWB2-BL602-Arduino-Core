/*
 * SoftwareSerial.h — bit-banged serial on any GPIO pair.
 *
 * A background FreeRTOS task samples the RX pin at mid-bit and pushes bytes
 * into a ring buffer; the sketch reads them via read()/available() exactly
 * like the hardware Serial. TX is a busy-wait bit-bang inside write().
 *
 * Timing comes from micros() (the free-running mtime counter), so it works
 * at 300..115200 baud. The RX task busy-polls while a frame is in flight
 * (~1 ms per byte at 9600 baud) but yields with taskYIELD() while the line
 * is idle, so the loop() task keeps running.
 */
#ifndef SoftwareSerial_h
#define SoftwareSerial_h

#include <Arduino.h>
#include "Stream.h"
#include <FreeRTOS.h>
#include <task.h>

#ifndef SW_SERIAL_RX_BUFFER_SIZE
#define SW_SERIAL_RX_BUFFER_SIZE 64
#endif

class SoftwareSerial : public Stream
{
public:
    SoftwareSerial(uint8_t rxPin, uint8_t txPin)
        : _rxPin(rxPin), _txPin(txPin), _bitTimeUs(104),
          _listening(false), _task(NULL), _head(0), _tail(0), _overflow(false) {}

    void begin(long speed);
    void end(void);

    bool listen(void) { _listening = true; return true; }
    bool isListening(void) { return _listening; }

    int available(void);
    int peek(void);
    int read(void);

    size_t write(uint8_t byte);
    using Print::write;

private:
    uint8_t _rxPin, _txPin;
    uint32_t _bitTimeUs;
    bool _listening;
    TaskHandle_t _task;

    volatile uint8_t _rxbuf[SW_SERIAL_RX_BUFFER_SIZE];
    volatile uint8_t _head, _tail; /* producer (task) / consumer (sketch) */
    bool _overflow;

    void _rxLoop(void); /* sample one byte, blocking */
    static void _taskFn(void *arg);
};

#endif /* SoftwareSerial_h */
