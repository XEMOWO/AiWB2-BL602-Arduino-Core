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
#include <sys/types.h> /* ssize_t (ESP8266-compatible streamRemaining()) */

#include "Print.h"
#include "WString.h"
#include "PolledTimeout.h" /* oneShotMs (esp8266::polledTimeout) for send*() */

#define NO_SKIP_CHAR 255 /* sentinel: skip only whitespace, like AVR */

class Stream : public Print
{
public:
    virtual int available(void) = 0;
    virtual int read(void) = 0;
    virtual int peek(void) = 0;
    virtual void flush(void) { }

    /* ESP8266 Stream API surface that third-party libraries rely on.
     * `read(buf,len)` has a default that fills via read(); StreamString and
     * the peek-buffer implementations (e.g. WiFiClient) override these. */
    virtual int read(uint8_t *buffer, size_t len);   /* default impl in Stream.cpp */
    virtual int read(char *buffer, size_t len)
    { return read((uint8_t *)buffer, len); }

    virtual bool hasPeekBufferAPI() const { return false; }
    virtual size_t peekAvailable()        { return 0; }
    virtual const char *peekBuffer()      { return nullptr; }
    virtual void peekConsume(size_t consume) { (void)consume; }

    virtual bool inputCanTimeout()        { return true; }
    virtual ssize_t streamRemaining()     { return -1; }

    void setTimeout(unsigned long timeout) { _timeout = timeout; }
    unsigned long getTimeout(void) const    { return _timeout; }

    /* ESP8266 "Streaming streams to streams" extension: send*() moves data from
     * this Stream into another Stream/Print with 1-copy peekBuffer transfers
     * when available.  Used by ESP8266WebServer, ESP8266HTTPClient and the
     * StreamDev helper classes.  Ported verbatim from cores/esp8266/Stream.h
     * (StreamSend.cpp implements the sendGeneric core). */
    using oneShotMs = esp8266::polledTimeout::oneShotFastMs;
    static constexpr int temporaryStackBufferSize = 64;

    size_t sendAvailable(Stream* to) { return sendGeneric(to, -1, -1, oneShotMs::alwaysExpired); }
    size_t sendAvailable(Stream& to) { return sendAvailable(&to); }
    size_t sendAvailable(Stream&& to) { return sendAvailable(&to); }

    size_t sendAll(Stream* to, const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendGeneric(to, -1, -1, timeoutMs); }
    size_t sendAll(Stream& to, const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendAll(&to, timeoutMs); }
    size_t sendAll(Stream&& to, const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendAll(&to, timeoutMs); }

    size_t sendUntil(Stream* to, const int readUntilChar,
                     const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendGeneric(to, -1, readUntilChar, timeoutMs); }
    size_t sendUntil(Stream& to, const int readUntilChar,
                     const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendUntil(&to, readUntilChar, timeoutMs); }
    size_t sendUntil(Stream&& to, const int readUntilChar,
                     const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendUntil(&to, readUntilChar, timeoutMs); }

    size_t sendSize(Stream* to, const ssize_t maxLen,
                    const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendGeneric(to, maxLen, -1, timeoutMs); }
    size_t sendSize(Stream& to, const ssize_t maxLen,
                    const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendSize(&to, maxLen, timeoutMs); }
    size_t sendSize(Stream&& to, const ssize_t maxLen,
                    const oneShotMs::timeType timeoutMs = oneShotMs::neverExpires)
    { return sendSize(&to, maxLen, timeoutMs); }

    enum class Report
    {
        Success = 0,
        TimedOut,
        ReadError,
        WriteError,
        ShortOperation,
    };

    Report getLastSendReport() const { return _sendReport; }

protected:
    size_t sendGeneric(Stream* to,
                       const ssize_t len = -1,
                       const int readUntilChar = -1,
                       oneShotMs::timeType timeoutMs = oneShotMs::neverExpires);

    size_t sendGeneric(Print* to,
                       const ssize_t len = -1,
                       const int readUntilChar = -1,
                       oneShotMs::timeType timeoutMs = oneShotMs::neverExpires);

    size_t SendGenericPeekBuffer(Print* to, const ssize_t len, const int readUntilChar,
                                 const oneShotMs::timeType timeoutMs);
    size_t SendGenericRegularUntil(Print* to, const ssize_t len, const int readUntilChar,
                                   const oneShotMs::timeType timeoutMs);
    size_t SendGenericRegular(Print* to, const ssize_t len,
                              const oneShotMs::timeType timeoutMs);

    void setReport(Report report) { _sendReport = report; }

    Report _sendReport = Report::Success;

public:
    /* Above: send-API internals are protected (matches ESP8266). Everything
     * below is the public Stream helper API — find/parse/readBytes/readString
     * are called by third-party libs (HTTPClient) through Client* handles. */

    bool find(const char *target)                        { return find(target, strlen(target)); }
    bool find(const char *target, size_t length);
    bool find(const String &target)                      { return find(target.c_str(), target.length()); }

    bool findUntil(const char *target, const char *terminator);
    bool findUntil(const String &target, const String &terminator);

    long  parseInt(void)                    { return parseInt(NO_SKIP_CHAR); }
    long  parseInt(char skipChar);
    float parseFloat(void)                  { return parseFloat(NO_SKIP_CHAR); }
    float parseFloat(char skipChar);

    virtual size_t readBytes(char *buffer, size_t length) { return readBytes((uint8_t *)buffer, length); }
    virtual size_t readBytes(uint8_t *buffer, size_t length);
    size_t readBytesUntil(char terminator, char *buffer, size_t length)
    { return readBytesUntil(terminator, (uint8_t *)buffer, length); }
    size_t readBytesUntil(char terminator, uint8_t *buffer, size_t length);

    virtual String readString(void);
    String readStringUntil(char terminator);

protected:
    Stream() : _timeout(1000) {}
    int timedRead(void);
    int timedPeek(void);

    unsigned long _timeout;
};

#endif /* Stream_h */
