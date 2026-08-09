/*
 * Print.cpp — Arduino Print base class implementation.
 */
#include "Print.h"
#include <stdio.h>   /* vsnprintf for Print::printf */
#include <string.h>
#include <stdarg.h>

size_t Print::write(const uint8_t *buf, size_t size)
{
    size_t n = 0;
    for (size_t i = 0; i < size; i++) {
        n += write(buf[i]);
    }
    return n;
}

size_t Print::write(const char *str)
{
    if (!str) {
        return 0;
    }
    size_t n = strlen(str);
    return write((const uint8_t *)str, n);
}

/* ---- number formatting ------------------------------------------------ */

size_t Print::printNumber(unsigned long long value, uint8_t base)
{
    char buf[8 * sizeof(unsigned long long) + 1];
    char *p = &buf[sizeof(buf) - 1];
    size_t n = 0;

    *p = '\0';
    do {
        unsigned int digit = (unsigned int)(value % base);
        *--p = (digit < 10) ? (char)('0' + digit) : (char)('A' + digit - 10);
        value /= base;
    } while (value != 0);

    n = write((const uint8_t *)p, strlen(p));
    return n;
}

size_t Print::printFloat(double value, uint8_t precision)
{
    if (precision > 6) {
        precision = 6;
    }
    size_t n = 0;

    /* sign */
    if (value < 0.0) {
        n += write('-');
        value = -value;
    }

    /* integer part + rounding-aware fractional part */
    double ip = (double)(long)value;
    double frac = value - ip;

    if (precision > 0) {
        double scale = 1.0;
        for (uint8_t i = 0; i < precision; i++) {
            scale *= 10.0;
        }
        unsigned long long frac_i = (unsigned long long)(frac * scale + 0.5);
        /* carry from rounding into the integer part */
        if (frac_i >= (unsigned long long)scale) {
            ip += 1.0;
            frac_i -= (unsigned long long)scale;
        }

        n += printNumber((unsigned long long)ip, DEC);
        n += write('.');
        /* zero-pad the fraction */
        unsigned long long div = scale / 10;
        while (div > 0) {
            n += write((char)('0' + (frac_i / div) % 10));
            div /= 10;
        }
    } else {
        n += printNumber((unsigned long long)(ip + 0.5), DEC);
    }
    return n;
}

/* ---- print() overloads ------------------------------------------------ */

size_t Print::print(const String &s)
{
    return write((const uint8_t *)s.c_str(), s.length());
}

size_t Print::print(const char str[])
{
    return write(str);
}

size_t Print::print(char c)
{
    return write((uint8_t)c);
}

size_t Print::print(unsigned char value, int base)
{
    return printNumber(value, (uint8_t)base);
}

size_t Print::print(int value, int base)
{
    if (base == DEC && value < 0) {
        size_t n = write('-');
        return n + printNumber((unsigned long)(-(long)value), base);
    }
    return printNumber(value, (uint8_t)base);
}

size_t Print::print(unsigned int value, int base)
{
    return printNumber(value, (uint8_t)base);
}

size_t Print::print(long value, int base)
{
    if (base == DEC && value < 0) {
        size_t n = write('-');
        return n + printNumber((unsigned long)(-(long)value), base);
    }
    return printNumber(value, (uint8_t)base);
}

size_t Print::print(unsigned long value, int base)
{
    return printNumber(value, (uint8_t)base);
}

size_t Print::print(long long value, int base)
{
    if (base == DEC && value < 0) {
        size_t n = write('-');
        return n + printNumber((unsigned long long)(-(long long)value), base);
    }
    return printNumber(value, (uint8_t)base);
}

size_t Print::print(unsigned long long value, int base)
{
    return printNumber(value, (uint8_t)base);
}

size_t Print::print(double value, int precision)
{
    return printFloat(value, (uint8_t)precision);
}

size_t Print::print(const void *p)
{
    size_t n = write("0x");
    return n + printNumber((unsigned long)(uintptr_t)p, HEX);
}

size_t Print::print(const Printable &p)
{
    return p.printTo(*this);
}

/* ---- println() overloads ---------------------------------------------- */

size_t Print::println(const char str[])
{
    size_t n = print(str);
    return n + println();
}

size_t Print::println(char c)
{
    size_t n = print(c);
    return n + println();
}

size_t Print::println(unsigned char value, int base)
{
    size_t n = print(value, base);
    return n + println();
}

size_t Print::println(int value, int base)
{
    size_t n = print(value, base);
    return n + println();
}

size_t Print::println(unsigned int value, int base)
{
    size_t n = print(value, base);
    return n + println();
}

size_t Print::println(long value, int base)
{
    size_t n = print(value, base);
    return n + println();
}

size_t Print::println(unsigned long value, int base)
{
    size_t n = print(value, base);
    return n + println();
}

size_t Print::println(long long value, int base)
{
    size_t n = print(value, base);
    return n + println();
}

size_t Print::println(unsigned long long value, int base)
{
    size_t n = print(value, base);
    return n + println();
}

size_t Print::println(double value, int precision)
{
    size_t n = print(value, precision);
    return n + println();
}

size_t Print::println(const void *p)
{
    size_t n = print(p);
    return n + println();
}

size_t Print::println(const Printable &p)
{
    size_t n = print(p);
    return n + println();
}

size_t Print::println(const String &s)
{
    size_t n = print(s);
    return n + println();
}

size_t Print::println(void)
{
    return write((const uint8_t *)"\r\n", 2);
}

int Print::printf(const char *format, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    if (n < 0) {
        return n;
    }
    if ((size_t)n >= sizeof(buf)) {
        n = (int)sizeof(buf) - 1; /* truncated */
    }
    write((const uint8_t *)buf, (size_t)n);
    return n;
}
