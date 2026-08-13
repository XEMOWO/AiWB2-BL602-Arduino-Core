/*
 * dtostrf.c — float-to-string formatter (AVR-libc compatible).
 *
 * avr-libc's dtostrf() is the de-facto reference (ESP8266 reuses it). It
 * formats `val` with `prec` decimal places and right-justifies the result in a
 * field of `width` characters; a negative `width` left-justifies (no padding).
 * NaN/Inf are not handled specially (avr-libc doesn't either for float).
 *
 * Deliberately avoids pow()/newlib %f so the core stays lean under
 * -ffreestanding and links without _printf_float.
 */
#include "Arduino.h"
#include <string.h>

char *dtostrf(double val, signed char width, unsigned char prec, char *sout)
{
    char buf[24];
    int len = 0;
    int pad;

    /* sign handling */
    if (val < 0.0) {
        buf[len++] = '-';
        val = -val;
    }

    /* scale by 10^prec and round, then split integer/fractional */
    {
        unsigned long scale = 1;
        unsigned long whole, frac;

        for (unsigned char i = 0; i < prec; i++) {
            scale *= 10;
        }

        /* Rounding: adding 0.5 to scaled value. Guard against overflow for
         * huge values (double here is single-precision float on this target,
         * so values beyond ~16M lose integer precision anyway). */
        if (val > 16777216.0 / (double)scale) {
            whole = (unsigned long)(val / (double)scale);
            frac = 0;
        } else {
            unsigned long scaled = (unsigned long)(val * (double)scale + 0.5);
            whole = scaled / scale;
            frac = scaled % scale;
        }

        /* integer digits (reverse into temp) */
        {
            char tmp[12];
            int t = 0;
            do {
                tmp[t++] = (char)('0' + (whole % 10));
                whole /= 10;
            } while (whole);
            while (t > 0) {
                buf[len++] = tmp[--t];
            }
        }

        /* fractional digits with leading zeros */
        if (prec) {
            unsigned long div = scale / 10;
            buf[len++] = '.';
            for (unsigned char i = 0; i < prec; i++) {
                buf[len++] = (char)('0' + ((frac / div) % 10));
                div /= 10;
            }
        }
    }
    buf[len] = '\0';

    /* right-justify into `width` if it exceeds the formatted length */
    pad = width - len;
    while (pad > 0) {
        for (int i = len; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = ' ';
        len++;
        pad--;
    }

    memcpy(sout, buf, (size_t)len + 1);
    return sout;
}
