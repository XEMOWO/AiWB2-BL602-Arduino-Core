/*
 * cbuf.cpp — circular byte buffer (ESP8266-compatible).
 *
 * Ported 1:1 from cores/esp8266/cbuf.cpp of the esp8266/Arduino repo
 * (Copyright (c) 2014 Ivan Grokhotkov, LGPL 2.1+).
 */
#include "cbuf.h"

cbuf::cbuf(size_t size) : next(NULL)
{
    _size = size;
    _buf = new char[size];
    _bufend = _buf + size;
    _begin = _buf;
    _end = _buf;
}

cbuf::~cbuf()
{
    delete[] _buf;
}

size_t cbuf::resizeAdd(size_t addSize)
{
    return resize(_size + addSize);
}

size_t cbuf::resize(size_t newSize)
{
    size_t bytes_available = available();
    if (newSize < bytes_available) {
        return _size;
    }

    char* newbuf = new char[newSize];
    for (size_t i = 0; i < bytes_available; i++) {
        newbuf[i] = read();
    }
    delete[] _buf;
    _buf = newbuf;
    _bufend = _buf + newSize;
    _begin = _buf;
    _end = _buf + bytes_available;
    _size = newSize;

    return _size;
}

size_t cbuf::available() const
{
    if (_end >= _begin) {
        return _end - _begin;
    }
    return _size - (_begin - _end);
}

size_t cbuf::size()
{
    return _size;
}

size_t cbuf::room() const
{
    if (_end >= _begin) {
        return _size - (_end - _begin);
    }
    return _begin - _end - 1;
}

int cbuf::peek()
{
    if (empty()) {
        return -1;
    }

    return static_cast<unsigned char>(*_begin);
}

size_t cbuf::peek(char *dst, size_t size)
{
    size_t bytes_available = available();
    size_t size_to_peek = (size < bytes_available) ? size : bytes_available;

    size_t size_peeked = 0;
    char* begin = _begin;
    while (size_peeked < size_to_peek) {
        *dst = *begin;
        dst++;
        begin = wrap_if_bufend(begin + 1);
        size_peeked++;
    }
    return size_peeked;
}

int cbuf::read()
{
    if (empty()) {
        return -1;
    }

    char c = *_begin;
    _begin = wrap_if_bufend(_begin + 1);
    return static_cast<unsigned char>(c);
}

size_t cbuf::read(char* dst, size_t size)
{
    size_t bytes_available = available();
    size_t size_to_read = (size < bytes_available) ? size : bytes_available;

    size_t size_read = 0;
    while (size_read < size_to_read) {
        *dst = read();
        dst++;
        size_read++;
    }
    return size_read;
}

size_t cbuf::write(char c)
{
    if (full()) {
        return 0;
    }

    *_end = c;
    _end = wrap_if_bufend(_end + 1);
    return 1;
}

size_t cbuf::write(const char* src, size_t size)
{
    size_t bytes_available = room();
    size_t size_to_write = (size < bytes_available) ? size : bytes_available;

    size_t size_written = 0;
    while (size_written < size_to_write) {
        (void)write(*src); /* the ESP8266 original assigned to *src here (bug) */
        src++;
        size_written++;
    }
    return size_written;
}

void cbuf::flush()
{
    _begin = _buf;
    _end = _buf;
}

size_t cbuf::remove(size_t size)
{
    size_t bytes_available = available();
    if (size >= bytes_available) {
        flush();
        return bytes_available;
    }

    size_t size_to_remove = size;
    while (size_to_remove--) {
        read();
    }
    return size;
}
