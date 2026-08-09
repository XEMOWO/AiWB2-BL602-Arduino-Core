/*
 * WString.cpp — String class implementation. All number formatting is done by
 * hand so the core never depends on newlib %f / %lld.
 */
#include "WString.h"

/* ---- helpers ----------------------------------------------------------- */

static char s_tolower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

static char s_toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - ('a' - 'A')) : c;
}

/* ---- constructors ------------------------------------------------------ */

String::String(void)
    : _buf(NULL), _len(0), _cap(0)
{
}

String::String(const char *cstr)
    : _buf(NULL), _len(0), _cap(0)
{
    if (cstr) {
        concat(cstr);
    }
}

String::String(const char *cstr, unsigned int length)
    : _buf(NULL), _len(0), _cap(0)
{
    if (cstr) {
        concat(cstr, length);
    }
}

String::String(const String &other)
    : _buf(NULL), _len(0), _cap(0)
{
    concat(other);
}

String::String(char c)
    : _buf(NULL), _len(0), _cap(0)
{
    concat(c);
}

String::String(unsigned char value, unsigned char base)
    : _buf(NULL), _len(0), _cap(0)
{
    concat((unsigned long)value, base);
}

String::String(int value, unsigned char base)
    : _buf(NULL), _len(0), _cap(0)
{
    concat((long)value, base);
}

String::String(unsigned int value, unsigned char base)
    : _buf(NULL), _len(0), _cap(0)
{
    concat((unsigned long)value, base);
}

String::String(long value, unsigned char base)
    : _buf(NULL), _len(0), _cap(0)
{
    concat(value, base);
}

String::String(unsigned long value, unsigned char base)
    : _buf(NULL), _len(0), _cap(0)
{
    concat(value, base);
}

String::String(float value, unsigned char decimalPlaces)
    : _buf(NULL), _len(0), _cap(0)
{
    concat((double)value, decimalPlaces);
}

String::String(double value, unsigned char decimalPlaces)
    : _buf(NULL), _len(0), _cap(0)
{
    concat(value, decimalPlaces);
}

String::~String(void)
{
    free(_buf);
    _buf = NULL;
    _len = 0;
    _cap = 0;
}

/* ---- buffer management -------------------------------------------------- */

unsigned char String::changeBuffer(unsigned int size)
{
    char *nb = (char *)realloc(_buf, size);
    if (!nb) {
        return 0;
    }
    _buf = nb;
    _cap = size;
    return 1;
}

unsigned char String::reserve(unsigned int size)
{
    if (_buf && _cap >= size) {
        return 1;
    }
    if (!changeBuffer(size)) {
        return 0;
    }
    _buf[0] = '\0';
    _len = 0;
    return 1;
}

/* Append n bytes; keeps the buffer null-terminated. */
unsigned char String::appendBuffer(const char *data, unsigned int n)
{
    if (n == 0) {
        return 1;
    }
    if (_len + n + 1 > _cap) {
        unsigned int newcap = _cap ? _cap : 16;
        while (newcap < _len + n + 1) {
            newcap <<= 1;
        }
        if (!changeBuffer(newcap)) {
            return 0;
        }
    }
    memcpy(_buf + _len, data, n);
    _len += n;
    _buf[_len] = '\0';
    return 1;
}

/* ---- concat ------------------------------------------------------------- */

unsigned char String::concat(const String &s) { return appendBuffer(s._buf, s._len); }
unsigned char String::concat(const char *cstr)
{
    if (!cstr) {
        return 0;
    }
    return appendBuffer(cstr, (unsigned int)strlen(cstr));
}
unsigned char String::concat(const char *cstr, unsigned int length)
{
    if (!cstr) {
        return 0;
    }
    return appendBuffer(cstr, length);
}
unsigned char String::concat(char c) { return appendBuffer(&c, 1); }
unsigned char String::concat(unsigned char num, unsigned char base) { return concat((unsigned long)num, base); }
unsigned char String::concat(int num, unsigned char base)           { return concat((long)num, base); }
unsigned char String::concat(unsigned int num, unsigned char base)  { return concat((unsigned long)num, base); }
unsigned char String::concat(long num, unsigned char base)
{
    char tmp[34];
    int  i = 0;
    if (num < 0 && base == 10) {
        tmp[i++] = '-';
        num = -num;
    }
    /* render unsigned magnitude */
    char digits[33];
    int  d = 0;
    unsigned long v = (unsigned long)num;
    if (v == 0) {
        digits[d++] = '0';
    }
    while (v) {
        int r = (int)(v % base);
        digits[d++] = (char)((r < 10) ? ('0' + r) : ('A' + r - 10));
        v /= base;
    }
    while (d > 0) {
        tmp[i++] = digits[--d];
    }
    tmp[i] = '\0';
    return appendBuffer(tmp, (unsigned int)i);
}
unsigned char String::concat(unsigned long num, unsigned char base) { return concat((long)num, base); }

unsigned char String::concat(float num, unsigned char dp)  { return concat((double)num, dp); }
unsigned char String::concat(double num, unsigned char dp) { return appendFloat(num, dp); }

/* ---- assignment ---------------------------------------------------------- */

String &String::operator=(const String &other)
{
    if (this != &other) {
        _len = 0;
        if (_buf) {
            _buf[0] = '\0';
        }
        concat(other);
    }
    return *this;
}

String &String::operator=(const char *cstr)
{
    _len = 0;
    if (_buf) {
        _buf[0] = '\0';
    }
    concat(cstr);
    return *this;
}

String &String::operator=(char c)
{
    _len = 0;
    if (_buf) {
        _buf[0] = '\0';
    }
    concat(c);
    return *this;
}

/* ---- character access / comparison -------------------------------------- */

char String::charAt(unsigned int index) const
{
    return (index < _len) ? _buf[index] : 0;
}

void String::setCharAt(unsigned int index, char c)
{
    if (index < _len) {
        _buf[index] = c;
    }
}

int String::compareTo(const String &s) const { return strcmp(_buf ? _buf : "", s._buf ? s._buf : ""); }
int String::compareTo(const char *cstr) const { return strcmp(_buf ? _buf : "", cstr ? cstr : ""); }

unsigned char String::equals(const String &s) const { return compareTo(s) == 0; }
unsigned char String::equals(const char *cstr) const { return compareTo(cstr) == 0; }

unsigned char String::equalsIgnoreCase(const String &s) const
{
    const char *a = _buf ? _buf : "";
    const char *b = s._buf ? s._buf : "";
    while (*a && *b) {
        if (s_tolower(*a) != s_tolower(*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

unsigned char String::startsWith(const String &prefix, unsigned int offset) const
{
    if (offset > _len) {
        return 0;
    }
    return strncmp(_buf + offset, prefix._buf, prefix._len) == 0;
}

unsigned char String::endsWith(const String &suffix) const
{
    if (suffix._len > _len) {
        return 0;
    }
    return strcmp(_buf + _len - suffix._len, suffix._buf) == 0;
}

/* ---- searching ------------------------------------------------------------ */

int String::indexOf(char ch, unsigned int fromIndex) const
{
    if (fromIndex >= _len) {
        return -1;
    }
    const char *p = strchr(_buf + fromIndex, ch);
    return p ? (int)(p - _buf) : -1;
}

int String::indexOf(const String &str, unsigned int fromIndex) const
{
    if (fromIndex >= _len) {
        return -1;
    }
    const char *p = strstr(_buf + fromIndex, str._buf);
    return p ? (int)(p - _buf) : -1;
}

int String::lastIndexOf(char ch) const
{
    if (!_len) {
        return -1;
    }
    const char *p = strrchr(_buf, ch);
    return p ? (int)(p - _buf) : -1;
}

int String::lastIndexOf(const String &str) const { return lastIndexOf(str, _len); }

int String::lastIndexOf(const String &str, unsigned int fromIndex) const
{
    if (!str._len || !_len || str._len > _len || fromIndex > _len) {
        return -1;
    }
    /* scan backwards for the pattern */
    int from = (int)(fromIndex - str._len);
    for (int i = from; i >= 0; i--) {
        if (strncmp(_buf + i, str._buf, str._len) == 0) {
            return i;
        }
    }
    return -1;
}

/* ---- substring / replace / case / trim ------------------------------------ */

String String::substring(unsigned int beginIndex) const
{
    return substring(beginIndex, _len);
}

String String::substring(unsigned int beginIndex, unsigned int endIndex) const
{
    if (beginIndex > _len) {
        beginIndex = _len;
    }
    if (endIndex > _len) {
        endIndex = _len;
    }
    if (endIndex < beginIndex) {
        return String();
    }
    return String(_buf + beginIndex, endIndex - beginIndex);
}

void String::replace(char find, char replace)
{
    for (unsigned int i = 0; i < _len; i++) {
        if (_buf[i] == find) {
            _buf[i] = replace;
        }
    }
}

void String::replace(const String &find, const String &replace)
{
    if (!find._len || find._len > _len) {
        return;
    }
    String out;
    unsigned int i = 0;
    while (i < _len) {
        if (i + find._len <= _len && strncmp(_buf + i, find._buf, find._len) == 0) {
            out.concat(replace);
            i += find._len;
        } else {
            out.concat(_buf[i]);
            i++;
        }
    }
    *this = out;
}

void String::toLowerCase(void)
{
    for (unsigned int i = 0; i < _len; i++) {
        _buf[i] = s_tolower(_buf[i]);
    }
}

void String::toUpperCase(void)
{
    for (unsigned int i = 0; i < _len; i++) {
        _buf[i] = s_toupper(_buf[i]);
    }
}

void String::trim(void)
{
    unsigned int s = 0, e = _len;
    while (s < e && (_buf[s] == ' ' || _buf[s] == '\t' || _buf[s] == '\r' || _buf[s] == '\n')) {
        s++;
    }
    while (e > s && (_buf[e - 1] == ' ' || _buf[e - 1] == '\t' || _buf[e - 1] == '\r' || _buf[e - 1] == '\n')) {
        e--;
    }
    if (s != 0 || e != _len) {
        unsigned int n = e - s;
        memmove(_buf, _buf + s, n);
        _len = n;
        _buf[n] = '\0';
    }
}

String &String::remove(unsigned int index, unsigned int count)
{
    if (index >= _len) {
        return *this;
    }
    if (count > _len - index) {
        count = _len - index;
    }
    memmove(_buf + index, _buf + index + count, _len - index - count + 1);
    _len -= count;
    return *this;
}

/* ---- conversions ----------------------------------------------------------- */

long String::toInt(void) const
{
    if (!_len) {
        return 0;
    }
    return strtol(_buf, NULL, 10);
}

float String::toFloat(void) const
{
    if (!_len) {
        return 0.0f;
    }
    return (float)strtod(_buf, NULL);
}

void String::getBytes(unsigned char *buf, unsigned int bufsize, unsigned int index) const
{
    if (!buf || !bufsize) {
        return;
    }
    unsigned int n = (index < _len) ? (_len - index) : 0;
    if (n >= bufsize) {
        n = bufsize - 1;
    }
    if (n) {
        memcpy(buf, _buf + index, n);
    }
    buf[n] = '\0';
}

/* ---- internal number formatting --------------------------------------------- */

/* Render a double with decimalPlaces decimals into this string. */
unsigned char String::appendFloat(double num, unsigned char decimalPlaces)
{
    char tmp[64];
    int  i = 0;

    if (num < 0) {
        tmp[i++] = '-';
        num = -num;
    }
    if (isnan(num) || isinf(num)) {
        tmp[i++] = 'n';
        tmp[i++] = 'a';
        tmp[i++] = 'n';
        tmp[i] = '\0';
        return appendBuffer(tmp, (unsigned int)i);
    }

    /* integer part */
    unsigned long ip = (unsigned long)num;
    unsigned char dp = decimalPlaces;
    if (dp > 10) {
        dp = 10;
    }

    /* fraction part, rounded to dp decimals */
    unsigned long m = 1;
    for (unsigned char k = 0; k < dp; k++) {
        m *= 10;
    }
    double frac = (num - (double)ip) * (double)m;
    unsigned long fp = (unsigned long)(frac + 0.5); /* round half away from zero */

    /* carry the rounded fraction into the integer part */
    if (fp >= m) {
        ip++;
        fp -= m;
    }

    /* render integer part digits */
    char idig[16];
    int  id = 0;
    if (ip == 0) {
        idig[id++] = '0';
    }
    while (ip) {
        idig[id++] = (char)('0' + ip % 10);
        ip /= 10;
    }
    while (id > 0) {
        tmp[i++] = idig[--id];
    }

    if (dp > 0) {
        tmp[i++] = '.';
        unsigned long div = m / 10;
        for (int k = (int)dp - 1; k >= 0; k--) {
            tmp[i++] = (char)('0' + (fp / div) % 10);
            div /= 10;
        }
    }
    tmp[i] = '\0';
    return appendBuffer(tmp, (unsigned int)i);
}

/* ---- operator+ --------------------------------------------------------------- */

String operator+(const String &a, const String &b)
{
    String r(a);
    r += b;
    return r;
}
String operator+(const String &a, const char *b) { String r(a); r += b; return r; }
String operator+(const char *a, const String &b) { String r(a); r += b; return r; }
String operator+(const String &a, char c) { String r(a); r += c; return r; }
String operator+(char a, const String &b) { String r(a); r += b; return r; }
