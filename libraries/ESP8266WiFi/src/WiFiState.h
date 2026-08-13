/*
  WiFiState.h - radio state snapshot for WiFi.shutdown()/resumeFromShutdown().

  On ESP8266 this captures the full station/AP SDK config. On BL602 the SDK
  owns the radio config (and stores the last AP in its PSM partition), so
  shutdown()/resumeFromShutdown() are accepted but effectively no-ops; this
  struct keeps the API shape so code that calls them compiles unchanged.
 */
#ifndef WIFISTATE_H_
#define WIFISTATE_H_

#include <stdint.h>
#include <ESP8266WiFiType.h>

struct WiFiState
{
    uint32_t crc;
    struct
    {
        WiFiMode_t mode;
        uint8_t channel;
        bool persistent;
    } state;
};

#endif /* WIFISTATE_H_ */
