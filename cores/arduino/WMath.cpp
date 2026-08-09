/*
 * WMath.cpp — map/constrain/random. This is a C++ file so our random(long)
 * overloads the POSIX random(void) declared in newlib's <stdlib.h> without a
 * conflicting-type error (C would not allow it).
 *
 * randomSeed(0) seeds the PRNG from the chip's TRNG (hosal_rng).
 */
#include "Arduino.h"

#include <stdlib.h>
#include <math.h>
#include <hosal_rng.h>

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long constrain(long amt, long low, long high)
{
    return (amt < low) ? low : ((amt > high) ? high : amt);
}

void randomSeed(unsigned long seed)
{
    if (seed == 0) {
        /* seed from hardware TRNG */
        uint32_t r = 0;
        hosal_rng_init();
        hosal_random_num_read(&r, sizeof(r));
        seed = (unsigned long)r;
    }
    srand((unsigned int)seed);
}

long random(long howbig)
{
    if (howbig <= 0) {
        return 0;
    }
    return (long)(rand() % (unsigned long)howbig);
}

long random(long howsmall, long howbig)
{
    if (howsmall >= howbig) {
        return howsmall;
    }
    long diff = howbig - howsmall;
    return howsmall + random(diff);
}
