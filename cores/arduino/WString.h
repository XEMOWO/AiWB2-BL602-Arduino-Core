/*
 * WString.h — Arduino String class (from-scratch implementation).
 *
 * Dynamic, null-terminated string with the usual Arduino semantics:
 * concatenation, searching, substring, replace, case folding, toInt/toFloat.
 * Numbers are formatted by hand — newlib's %f / %lld support is not
 * guaranteed in the SDK's build, so we never rely on it.
 */
#ifndef WString_h
#define WString_h

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

class String
{
public:
    String(void);
    String(const char *cstr);
    String(const char *cstr, unsigned int length);
    String(const String &other);
    String(char c);
    String(unsigned char value, unsigned char base = 10);
    String(int value, unsigned char base = 10);
    String(unsigned int value, unsigned char base = 10);
    String(long value, unsigned char base = 10);
    String(unsigned long value, unsigned char base = 10);
    String(float value, unsigned char decimalPlaces = 2);
    String(double value, unsigned char decimalPlaces = 2);
    ~String(void);

    unsigned char reserve(unsigned int size);
    unsigned int  length(void) const { return _len; }
    unsigned int  capacity(void) const { return _cap; }

    unsigned char concat(const String &s);
    unsigned char concat(const char *cstr);
    unsigned char concat(const char *cstr, unsigned int length);
    unsigned char concat(char c);
    unsigned char concat(unsigned char num, unsigned char base = 10);
    unsigned char concat(int num, unsigned char base = 10);
    unsigned char concat(unsigned int num, unsigned char base = 10);
    unsigned char concat(long num, unsigned char base = 10);
    unsigned char concat(unsigned long num, unsigned char base = 10);
    unsigned char concat(float num, unsigned char decimalPlaces = 2);
    unsigned char concat(double num, unsigned char decimalPlaces = 2);

    String &operator=(const String &other);
    String &operator=(const char *cstr);
    String &operator=(char c);

    String &operator+=(const String &other) { concat(other); return *this; }
    String &operator+=(const char *cstr)     { concat(cstr);  return *this; }
    String &operator+=(char c)               { concat(c);     return *this; }
    String &operator+=(unsigned char n)      { concat(n);     return *this; }
    String &operator+=(int n)                { concat(n);     return *this; }
    String &operator+=(unsigned int n)       { concat(n);     return *this; }
    String &operator+=(long n)               { concat(n);     return *this; }
    String &operator+=(unsigned long n)      { concat(n);     return *this; }
    String &operator+=(float n)              { concat(n);     return *this; }
    String &operator+=(double n)             { concat(n);     return *this; }

    char &operator[](unsigned int index)       { return _buf[index]; }
    char  operator[](unsigned int index) const { return _buf[index]; }

    char charAt(unsigned int index) const;
    void setCharAt(unsigned int index, char c);
    int  compareTo(const String &s) const;
    int  compareTo(const char *cstr) const;
    unsigned char equals(const String &s) const;
    unsigned char equals(const char *cstr) const;
    unsigned char equalsIgnoreCase(const String &s) const;
    unsigned char startsWith(const String &prefix, unsigned int offset) const;
    unsigned char startsWith(const String &prefix) const { return startsWith(prefix, 0); }
    unsigned char endsWith(const String &suffix) const;

    int indexOf(char ch) const                       { return indexOf(ch, 0); }
    int indexOf(char ch, unsigned int fromIndex) const;
    int indexOf(const String &str) const             { return indexOf(str, 0); }
    int indexOf(const String &str, unsigned int fromIndex) const;
    int lastIndexOf(char ch) const;
    int lastIndexOf(const String &str) const;
    int lastIndexOf(const String &str, unsigned int fromIndex) const;

    String substring(unsigned int beginIndex) const;
    String substring(unsigned int beginIndex, unsigned int endIndex) const;

    void replace(char find, char replace);
    void replace(const String &find, const String &replace);
    void toLowerCase(void);
    void toUpperCase(void);
    void trim(void);

    long   toInt(void) const;
    float  toFloat(void) const;

    const char *c_str(void) const { return _buf; }
    const char *begin(void) const { return _buf; }
    const char *end(void)   const { return _buf + _len; }
    char       *getBuffer(void)   { return _buf; } /* writable, for printf() */
    void getBytes(unsigned char *buf, unsigned int bufsize, unsigned int index = 0) const;
    String &remove(unsigned int index, unsigned int count);
    String &remove(unsigned int index) { return remove(index, _len - index); }

    unsigned char operator==(const String &s) const { return equals(s); }
    unsigned char operator!=(const String &s) const { return !equals(s); }
    unsigned char operator<(const String &s) const  { return compareTo(s) < 0; }
    unsigned char operator>(const String &s) const  { return compareTo(s) > 0; }
    unsigned char operator<=(const String &s) const { return compareTo(s) <= 0; }
    unsigned char operator>=(const String &s) const { return compareTo(s) >= 0; }

    /* comparison with a C string is covered by the String-returning
     * constructors, so only the String forms above are needed. */

private:
    char        *_buf;
    unsigned int _len;
    unsigned int _cap;

    unsigned char changeBuffer(unsigned int size);
    unsigned char appendBuffer(const char *data, unsigned int n);
    unsigned char appendNumber(unsigned long num, unsigned char base);
    unsigned char appendFloat(double num, unsigned char decimalPlaces);
};

/* operator+ concatenations */
String operator+(const String &a, const String &b);
String operator+(const String &a, const char *b);
String operator+(const char *a, const String &b);
String operator+(const String &a, char c);
String operator+(char a, const String &b);

#endif /* WString_h */
