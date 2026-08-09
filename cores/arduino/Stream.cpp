/*
 * Stream.cpp — text parsing on top of available()/read()/peek().
 */
#include "Stream.h"

#include "Arduino.h"

int Stream::timedRead(void)
{
    int c;
    unsigned long start = millis();

    while ((c = read()) < 0) {
        if ((millis() - start) >= _timeout) {
            return -1;
        }
    }
    return c;
}

int Stream::timedPeek(void)
{
    int c;
    unsigned long start = millis();

    while ((c = peek()) < 0) {
        if ((millis() - start) >= _timeout) {
            return -1;
        }
    }
    return c;
}

bool Stream::find(const char *target, size_t length)
{
    size_t index = 0;

    while (index < length) {
        int c = timedRead();
        if (c < 0) {
            return false;
        }
        if (c == (uint8_t)target[index]) {
            index++;
        } else {
            index = 0; /* restart the match on this byte */
            if (c == (uint8_t)target[0]) {
                index = 1;
            }
        }
    }
    return true;
}

bool Stream::findUntil(const char *target, const char *terminator)
{
    size_t tlen = strlen(target);
    size_t ttlen = strlen(terminator);
    size_t index = 0;
    size_t tindex = 0;

    while (index < tlen) {
        int c = timedRead();
        if (c < 0) {
            return false;
        }
        if (c == (uint8_t)terminator[tindex]) {
            if (++tindex >= ttlen) {
                return false; /* terminator seen first */
            }
            index = 0;
        } else {
            tindex = 0;
            if (c == (uint8_t)target[index]) {
                index++;
            } else {
                index = 0;
                if (c == (uint8_t)target[0]) {
                    index = 1;
                }
            }
        }
    }
    return true;
}

bool Stream::findUntil(const String &target, const String &terminator)
{
    return findUntil(target.c_str(), terminator.c_str());
}

long Stream::parseInt(char skipChar)
{
    bool isNegative = false;
    long value = 0;
    int c;

    /* skip non-numeric leading bytes */
    c = timedPeek();
    while (c >= 0 && (c < '0' || c > '9') && c != '-') {
        bool skip = (skipChar == NO_SKIP_CHAR)
            ? (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f')
            : (c == skipChar);
        if (!skip) {
            break;
        }
        read();
        c = timedPeek();
    }

    if (c == '-') {
        isNegative = true;
        read();
        c = timedPeek();
    }

    while (c >= '0' && c <= '9') {
        value = value * 10 + (c - '0');
        read();
        c = timedPeek();
    }

    return isNegative ? -value : value;
}

float Stream::parseFloat(char skipChar)
{
    bool isNegative = false;
    float value = 0.0f;
    int c;

    c = timedPeek();
    while (c >= 0 && (c < '0' || c > '9') && c != '-' && c != '.') {
        bool skip = (skipChar == NO_SKIP_CHAR)
            ? (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f')
            : (c == skipChar);
        if (!skip) {
            break;
        }
        read();
        c = timedPeek();
    }

    if (c == '-') {
        isNegative = true;
        read();
        c = timedPeek();
    }

    while (c >= '0' && c <= '9') {
        value = value * 10.0f + (c - '0');
        read();
        c = timedPeek();
    }

    if (c == '.') {
        read();
        c = timedPeek();
        float divisor = 1.0f;
        while (c >= '0' && c <= '9') {
            divisor *= 10.0f;
            value += (c - '0') / divisor;
            read();
            c = timedPeek();
        }
    }

    return isNegative ? -value : value;
}

size_t Stream::readBytes(uint8_t *buffer, size_t length)
{
    size_t index = 0;
    while (index < length) {
        int c = timedRead();
        if (c < 0) {
            break;
        }
        buffer[index++] = (uint8_t)c;
    }
    return index;
}

size_t Stream::readBytesUntil(char terminator, uint8_t *buffer, size_t length)
{
    size_t index = 0;
    while (index < length) {
        int c = timedRead();
        if (c < 0 || c == terminator) {
            break;
        }
        buffer[index++] = (uint8_t)c;
    }
    return index;
}

String Stream::readString(void)
{
    String s;
    int c;
    while ((c = timedRead()) >= 0) {
        s += (char)c;
    }
    return s;
}

String Stream::readStringUntil(char terminator)
{
    String s;
    int c;
    while ((c = timedRead()) >= 0 && c != terminator) {
        s += (char)c;
    }
    return s;
}
