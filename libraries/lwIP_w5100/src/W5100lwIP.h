/*
    W5100lwIP.h — Wiznet W5100 lwIP interface for the Ai-WB2-12F (BL602).

    `Wiznet5100lwIP` is the ESP8266 alias for LwipIntfDev<Wiznet5100>. On WB2
    the template is a no-op (see cores/arduino/LwipIntfDev.h).
*/

#ifndef _W5100LWIP_H
#define _W5100LWIP_H

#include <LwipIntfDev.h>
#include <utility/w5100.h>

using Wiznet5100lwIP = LwipIntfDev<Wiznet5100>;

#endif  // _W5100LWIP_H
