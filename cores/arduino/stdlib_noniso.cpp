/*
  stdlib_noniso.h - nonstandard (but useful) conversion functions

  Copyright (c) 2021 David Gauchard. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "stdlib_noniso.h"

#include <string.h> /* strlen (strrstr) */

extern "C" {

// ulltoa() is slower than std::to_char() (1.6 times)
// but is smaller by ~800B/flash and ~250B/rodata

// ulltoa fills str backwards and can return a pointer different from str
char* ulltoa(unsigned long long val, char* str, int slen, unsigned int radix) noexcept
{
    str += --slen;
    *str = 0;
    do
    {
        auto mod = val % radix;
        val /= radix;
        *--str = mod + ((mod > 9) ? ('a' - 10) : '0');
    } while (--slen && val);
    return val? nullptr: str;
}

// lltoa fills str backwards and can return a pointer different from str
char* lltoa(long long val, char* str, int slen, unsigned int radix) noexcept
{
    bool neg;
    if (val < 0)
    {
        val = -val;
        neg = true;
    }
    else
    {
        neg = false;
    }
    char* ret = ulltoa(val, str, slen, radix);
    if (neg)
    {
        if (ret == str || ret == nullptr)
            return nullptr;
        *--ret = '-';
    }
    return ret;
}

char* ltoa(long value, char* result, int base) noexcept {
    return itoa((int)value, result, base);
}

char* ultoa(unsigned long value, char* result, int base) noexcept {
    return utoa((unsigned int)value, result, base);
}

// strrstr — backwards search for p_pcPattern in p_pcString.
// Needed by the LEAmDNS (ESP8266mDNS) port; the ESP8266 core defines it here.
const char* strrstr(const char*__restrict p_pcString,
                    const char*__restrict p_pcPattern)
{
    const char* pcResult = 0;

    size_t      stStringLength = (p_pcString ? strlen(p_pcString) : 0);
    size_t      stPatternLength = (p_pcPattern ? strlen(p_pcPattern) : 0);

    if ((stStringLength) &&
            (stPatternLength) &&
            (stPatternLength <= stStringLength))
    {
        // Pattern is shorter or has the same length than the string
        for (const char* s = (p_pcString + stStringLength - stPatternLength); s >= p_pcString; --s)
        {
            if (0 == strncmp(s, p_pcPattern, stPatternLength))
            {
                pcResult = s;
                break;
            }
        }
    }
    return pcResult;
}

} // extern "C"
