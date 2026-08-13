/*
 * EthernetClient.h - classic Arduino Ethernet client for the Ai-WB2-12F (BL602).
 *
 * The WB2 has no Wiznet chip; the Ethernet API is a compile-compatible shim
 * that rides on the WiFi/lwIP stack, so EthernetClient is WiFiClient with an
 * Ethernet-compatible name (the classic API surface is a superset subset of
 * WiFiClient's). Sketches therefore compile unchanged and run over WiFi.
 */

#ifndef __ETHERNET_CLIENT_H__
#define __ETHERNET_CLIENT_H__

#include <WiFiClient.h>

typedef WiFiClient EthernetClient;

#endif /* __ETHERNET_CLIENT_H__ */
