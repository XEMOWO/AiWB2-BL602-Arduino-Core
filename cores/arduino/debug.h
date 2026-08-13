/*
 debug.h - debug print helpers (ESP8266-compatible, stubbed for WB2).
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.

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

 WB2 note: debug output compiled out by default to keep text size down.
 Define WIFI_DEBUG or ARDUINO_DEBUG to route DEBUGV to Serial.printf.
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG_ESP_PORT
#ifndef DEBUG_ESP_HTTP_CLIENT
#define DEBUG_ESP_HTTP_CLIENT 1
#endif
#endif

#ifdef DEBUG_ESP_PORT
#define DEBUGV(fmt, ...) DEBUG_ESP_PORT.printf_P(PSTR(fmt), ## __VA_ARGS__)
#else
#define DEBUGV(...) do {} while (0)
#endif

/* ESP8266 core's panic() hook, used by HwdtStackDump/IramReserve/MMU48K to
 * force a watchdog reset on demand. BL602 has no software-WDT reset wired here,
 * so __panic_func prints the fault site and hangs (noreturn) — the hardware
 * watchdog fires if enabled. Implemented in esp_compat.c. */
#ifdef __cplusplus
extern "C" {
#endif
void __panic_func(const char* file, int line, const char* func) __attribute__((noreturn));
#ifdef __cplusplus
}
#endif
#define panic() __panic_func(PSTR(__FILE__), __LINE__, __func__)

#endif /* DEBUG_H */
