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
#include <pgmspace.h> /* PROGMEM / F() — the ESP8266 WString.h includes this too, so headers that only pull WString.h (ESP8266WiFiMesh's JsonTranslator.h) still see PROGMEM */

class String
{
public:
    String(void);
    String(const char *cstr);
    String(const __FlashStringHelper *pstr); /* F("...") — same as const char* */
    String(const char *cstr, unsigned int length);
    String(const String &other);
    String(char c);
    String(unsigned char value, unsigned char base = 10);
    String(int value, unsigned char base = 10);
    String(unsigned int value, unsigned char base = 10);
    String(long value, unsigned char base = 10);
    String(unsigned long value, unsigned char base = 10);
    /* 64-bit overloads: newlib's `time_t` is __int_least64_t on RISC-V, so
     * `String(dir.fileTime())` (time_t) would otherwise be ambiguous among the
     * 32-bit ctors + double. The ESP8266 core needs no such overload (its
     * time_t is 32-bit); adding them here is a safe superset. */
    String(long long value, unsigned char base = 10);
    String(unsigned long long value, unsigned char base = 10);
    String(float value, unsigned char decimalPlaces = 2);
    String(double value, unsigned char decimalPlaces = 2);
    ~String(void);

    unsigned char reserve(unsigned int size);
    unsigned int  length(void) const { return _len; }
    unsigned int  capacity(void) const { return _cap; }

    bool isEmpty(void) const { return length() == 0; }  /* ESP8266 API */

    /* Empty the string, keeping the buffer. ESP8266 String::clear() — HTTPClient
     * and friends call it to reuse a response buffer across requests. */
    void clear(void)
    {
        if (_buf) {
            _buf[0] = '\0';
        }
        _len = 0;
    }

    /* ESP8266 String truthiness — HTTPClient tests `if (url && url[0] == '/')`.
     * explicit so it participates in contextual bool (if/&&/!) but never in
     * arithmetic or pointer contexts. */
    explicit operator bool() const { return _len != 0; }


    unsigned char concat(const String &s);
    unsigned char concat(const char *cstr);
    unsigned char concat(const __FlashStringHelper *str);
    unsigned char concat(const char *cstr, unsigned int length);
    unsigned char concat(char c);
    unsigned char concat(unsigned char num, unsigned char base = 10);
    unsigned char concat(int num, unsigned char base = 10);
    unsigned char concat(unsigned int num, unsigned char base = 10);
    unsigned char concat(long num, unsigned char base = 10);
    unsigned char concat(unsigned long num, unsigned char base = 10);
    unsigned char concat(long long num, unsigned char base = 10);
    unsigned char concat(unsigned long long num, unsigned char base = 10);
    unsigned char concat(float num, unsigned char decimalPlaces = 2);
    unsigned char concat(double num, unsigned char decimalPlaces = 2);

    String &operator=(const String &other);
    String &operator=(const char *cstr);
    String &operator=(const __FlashStringHelper *str);
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
    unsigned char equalsConstantTime(const String &s2) const;  /* ESP8266 API */
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
    /* ESP8266 core: mutable iterators — AEAD calls (ChaCha20Poly1305::encrypt/
     * decrypt) pass begin() into a void* for in-place encryption. */
    char       *begin(void)       { return _buf; }
    char       *end(void)         { return _buf + _len; }
    char       *getBuffer(void)   { return _buf; } /* writable, for printf() */
    void getBytes(unsigned char *buf, unsigned int bufsize, unsigned int index = 0) const;
    /* ESP8266 core alias used by DNSServer/CaptivePortalAdvanced: copy at most
     * buflen-1 chars plus a NUL terminator, or the full string if shorter. */
    void toCharArray(char *buf, unsigned int buflen) const {
        if (!buf) return;
        size_t n = _len < (size_t)buflen - 1 ? _len : (size_t)buflen - 1;
        memcpy(buf, _buf, n);
        buf[n] = '\0';
    }
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

/* The shared empty String, used as a default-argument sentinel in the ESP8266
 * WiFi/Server headers (e.g. `const String& psk = emptyString`). */
extern const String emptyString;

/* operator+ concatenations */
String operator+(const String &a, const String &b);
String operator+(const String &a, const char *b);
String operator+(const char *a, const String &b);
String operator+(const String &a, char c);
String operator+(char a, const String &b);

#endif /* WString_h */
