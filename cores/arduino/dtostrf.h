/*
 * dtostrf.h — float-to-string formatter, API-compatible with the AVR/ESP8266
 * avr/dtostrf.h. Some third-party libraries call dtostrf() directly.
 */
#ifndef dtostrf_h
#define dtostrf_h

#ifdef __cplusplus
extern "C" {
#endif

/* Format a double into sout with `prec` decimal places, right-justified in a
 * field of `width` (negative width = left-justified, no padding). Returns
 * sout. Implemented in dtostrf.c without libm/newlib %f (keeps -nostdlib
 * firmware lean). */
char *dtostrf(double val, signed char width, unsigned char prec, char *sout);

#ifdef __cplusplus
}
#endif

#endif /* dtostrf_h */
