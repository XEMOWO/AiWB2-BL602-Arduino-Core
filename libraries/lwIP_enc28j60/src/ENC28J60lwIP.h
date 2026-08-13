/*
    ENC28J60lwIP.h — Microchip ENC28J60 lwIP interface for the Ai-WB2-12F (BL602).

    `ENC28J60lwIP` is the ESP8266 alias for LwipIntfDev<ENC28J60>. On WB2 the
    template is a no-op (see cores/arduino/LwipIntfDev.h).
*/

#ifndef _ENC28J60LWIP_H
#define _ENC28J60LWIP_H

#include <LwipIntfDev.h>
#include <utility/enc28j60.h>

using ENC28J60lwIP = LwipIntfDev<ENC28J60>;

#endif  // _ENC28J60LWIP_H
