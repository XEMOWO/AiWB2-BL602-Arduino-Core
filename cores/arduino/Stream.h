/*
 * Stream.h — Arduino Stream base class (byte-oriented input on top of Print).
 *
 * Anything that implements available()/read()/peek() gets the standard text
 * parsing API: find/parseInt/parseFloat/readBytes/readString.
 */
#ifndef Stream_h
#define Stream_h

#include <stdint.h>
#include <stddef.h>

#include "Print.h"
#include "WString.h"

#define NO_SKIP_CHAR 255 /* sentinel: skip only whitespace, like AVR */

class Stream : public Print
{
public:
    virtual int available(void) = 0;
    virtual int read(void) = 0;
    virtual int peek(void) = 0;
    virtual void flush(void) { }

    void setTimeout(unsigned long timeout) { _timeout = timeout; }
    unsigned long getTimeout(void) const    { return _timeout; }

    bool find(const char *target)                        { return find(target, strlen(target)); }
    bool find(const char *target, size_t length);
    bool find(const String &target)                      { return find(target.c_str(), target.length()); }

    bool findUntil(const char *target, const char *terminator);
    bool findUntil(const String &target, const String &terminator);

    long  parseInt(void)                    { return parseInt(NO_SKIP_CHAR); }
    long  parseInt(char skipChar);
    float parseFloat(void)                  { return parseFloat(NO_SKIP_CHAR); }
    float parseFloat(char skipChar);

    size_t readBytes(char *buffer, size_t length)         { return readBytes((uint8_t *)buffer, length); }
    size_t readBytes(uint8_t *buffer, size_t length);
    size_t readBytesUntil(char terminator, char *buffer, size_t length)
    { return readBytesUntil(terminator, (uint8_t *)buffer, length); }
    size_t readBytesUntil(char terminator, uint8_t *buffer, size_t length);

    String readString(void);
    String readStringUntil(char terminator);

protected:
    Stream() : _timeout(1000) {}
    int timedRead(void);
    int timedPeek(void);

    unsigned long _timeout;
};

#endif /* Stream_h */
