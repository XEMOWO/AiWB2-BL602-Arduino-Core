/*
    esp_priv.h - private esp8266 helpers (BL602 port)
    Copyright (c) 2020 esp8266/Arduino community.  All right reserved.

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

    On Xtensa the DATA RAM lives in a different address space than the
    instruction/flash-mapped range, so the pgm_* helpers are needed for flash
    pointers.  On BL602 the whole address map (SRAM plus flash-mapped window)
    is byte-addressable by ordinary loads, so __byteAddressable() is always
    true and the pgm_* macros fall back to plain memcpy/loads (pgmspace.h).
*/

#ifndef __ESP_PRIV
#define __ESP_PRIV

inline bool __byteAddressable(const void*)
{
    return true;   /* BL602: RAM and mmap'ed flash are both byte-addressable */
}

#endif // __ESP_PRIV
