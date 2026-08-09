/*
 * Print.h — Arduino Print base class (minimal from-scratch implementation).
 *
 * Any class that implements write() becomes printable via print()/println().
 * Numbers are formatted by hand (no dependency on newlib's %f / long-long
 * printf support, which is not guaranteed in the SDK's newlib build).
 */
#ifndef Print_h
#define Print_h

#include <stddef.h>
#include <stdint.h>

#include "WString.h"

/* Number bases for print(x, base) */
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class Print; /* fwd decl for Printable */

/* A value that knows how to render itself to a Print (e.g. IPAddress). */
class Printable
{
public:
    virtual ~Printable() {}
    virtual size_t printTo(Print &p) const = 0;
};

class Print
{
public:
    /* Lowest level: write one byte. Subclasses implement this. */
    virtual size_t write(uint8_t c) = 0;

    /* Default: loop write() over the buffer. */
    virtual size_t write(const uint8_t *buf, size_t size);

    size_t write(const char *str); /* write() a C-string, ignoring terminator */

    size_t print(const String &s);
    size_t println(const String &s);

    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char value, int base = DEC);
    size_t print(int value, int base = DEC);
    size_t print(unsigned int value, int base = DEC);
    size_t print(long value, int base = DEC);
    size_t print(unsigned long value, int base = DEC);
    size_t print(long long value, int base = DEC);
    size_t print(unsigned long long value, int base = DEC);
    size_t print(double value, int precision = 2);
    size_t print(const void *p); /* pointer, printed as HEX */
    size_t print(const Printable &p); /* e.g. IPAddress */

    size_t println(const char[]);
    size_t println(char);
    size_t println(unsigned char value, int base = DEC);
    size_t println(int value, int base = DEC);
    size_t println(unsigned int value, int base = DEC);
    size_t println(long value, int base = DEC);
    size_t println(unsigned long value, int base = DEC);
    size_t println(long long value, int base = DEC);
    size_t println(unsigned long long value, int base = DEC);
    size_t println(double value, int precision = 2);
    size_t println(const void *p);
    size_t println(const Printable &p);
    size_t println(void); /* just a CR/LF */

    /* printf-style output (newlib vsnprintf; %f depends on the newlib build) */
    int printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

private:
    size_t printNumber(unsigned long long value, uint8_t base);
    size_t printFloat(double value, uint8_t precision);
};

#endif /* Print_h */
