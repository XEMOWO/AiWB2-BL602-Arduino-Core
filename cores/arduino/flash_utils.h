/*
 flash_utils.h - flash access data structures (ESP8266-compatible, BL602 shim).
 Copyright (c) 2015 Ivan Grokhotkov.  All right reserved.

 This file is part of the esp8266 core for Arduino environment.

 Ported to Ai-WB2-12F (BL602): on the ESP8266 this re-exports the eboot
 linker symbols (_FS_start, _FS_end, _FS_page, _FS_block) that mark the
 filesystem region in flash. BL602 resolves that region at runtime from the
 boot2 partition table (the "media" partition), so nothing is needed here —
 this is an empty guard header kept so `#include <flash_utils.h>` in LittleFS
 and third-party code compiles unchanged.
 */
#ifndef FLASH_UTILS_H
#define FLASH_UTILS_H

#endif /* FLASH_UTILS_H */
