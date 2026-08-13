/*
 * cbuf.h — circular byte buffer (ESP8266-compatible).
 *
 * Backs HardwareSerial/SoftwareSerial RX queues and network read buffers.
 * Ported 1:1 from cores/esp8266/cbuf.h of the esp8266/Arduino repo
 * (Copyright (c) 2014 Ivan Grokhotkov, LGPL 2.1+).
 */
#ifndef __cbuf_h
#define __cbuf_h

#include <stddef.h>
#include <stdint.h>
#include <string.h>

class cbuf {
    public:
        cbuf(size_t size);
        ~cbuf();

        size_t resizeAdd(size_t addSize);
        size_t resize(size_t newSize);
        size_t available() const;
        size_t size();

        size_t room() const;

        inline bool empty() const {
            return _begin == _end;
        }

        inline bool full() const {
            return wrap_if_bufend(_end + 1) == _begin;
        }

        int peek();
        size_t peek(char *dst, size_t size);

        int read();
        size_t read(char* dst, size_t size);

        size_t write(char c);
        size_t write(const char* src, size_t size);

        void flush();
        size_t remove(size_t size);

        cbuf *next;

    private:
        inline char* wrap_if_bufend(char* ptr) const {
            return (ptr == _bufend) ? _buf : ptr;
        }

        size_t _size;
        char* _buf;
        const char* _bufend;
        char* _begin;
        char* _end;
};

#endif /* __cbuf_h */
