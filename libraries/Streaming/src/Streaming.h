/*
 * Streaming.h - Arduino "Streaming" library for the Ai-WB2-12F (BL602).
 *
 * Faithful port of Mikal Hart's Streaming (arduino-libraries/Streaming): lets
 * sketches do `Serial << "value=" << x << endl`. Provides the global `stream`
 * object bound to Serial and the endl/flush manipulators.
 */

#ifndef Streaming_h
#define Streaming_h

#include <Arduino.h>
#include <Print.h>

/* ---- endl manipulator: Serial << "hi" << endl ----
 * (A `flush` macro is deliberately NOT provided: it would also replace the
 * Stream::flush() member call everywhere, breaking every `.flush()` in the
 * sketch. endl is what sketches actually use.) */
#define endl  (_endLine)

inline void _endLine(Print& p) { p.println(); }

class _Streamer
{
public:
    _Streamer(Print& s) : _s(&s) {}

    template <class T>
    _Streamer& operator << (const T& x) { _s->print(x); return *this; }

    _Streamer& operator << (const __FlashStringHelper* x) { _s->print(x); return *this; }

    _Streamer& operator << (void (*func)(Print&)) { func(*_s); return *this; }

private:
    Print* _s;
};

extern _Streamer stream;

/* Macro to bind a Streamer to another stream: DECLARE_STREAM(Serial1) */
#define DECLARE_STREAM(streamObject) _Streamer streamObject(streamObject##_object);

#endif /* Streaming_h */
