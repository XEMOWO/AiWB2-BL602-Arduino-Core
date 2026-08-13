
/*
    W5500lwIP.h — Wiznet W5500 lwIP interface for the Ai-WB2-12F (BL602).

    `Wiznet5500lwIP` is the ESP8266 alias for LwipIntfDev<Wiznet5500>. On WB2
    the template is a no-op (see cores/arduino/LwipIntfDev.h), so sketches that
    declare `Wiznet5500lwIP eth(CS)` compile unchanged; begin() reports no
    hardware.
*/

#ifndef _W5500LWIP_H
#define _W5500LWIP_H

#include <LwipIntfDev.h>
#include <utility/w5500.h>

using Wiznet5500lwIP = LwipIntfDev<Wiznet5500>;

#endif  // _W5500LWIP_H
