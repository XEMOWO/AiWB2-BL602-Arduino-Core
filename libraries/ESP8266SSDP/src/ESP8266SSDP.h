/*
 * ESP8266SSDP.h - ESP8266-compatible SSDP (UPnP discovery) for the WB2.
 *
 * Compile-compatible port. The reference responder answers M-SEARCH probes on
 * UDP 1900 and serves a device description.xml through the HTTP server.
 * BL602's lwIP has no SSDP module, so begin() reports success but no M-SEARCH
 * is ever answered. The property setters are stored so schema() can serve a
 * minimal description.xml through an HTTP server if one is running.
 *
 * API mirrors the reference ESP8266SSDP library.
 */

#ifndef ESP8266SSDP_H
#define ESP8266SSDP_H

#include <Arduino.h>
#include <Stream.h>

class WiFiClient;

class SSDPClass
{
public:
    SSDPClass() {}

    /* Start advertising. */
    bool begin()
    {
        return true;
    }

    /* Serve the device description to `client` (the HTTP server passes its
     * active client here). Emits a minimal SSDP description.xml. */
    bool schema(WiFiClient &client);

    /* ---- device description properties ---- */
    void setSchemaURL(const char *url)          { _schemaURL = url; }
    void setDeviceType(const char *type)        { _deviceType = type; }
    void setModelName(const char *name)         { _modelName = name; }
    void setModelNumber(const char *num)        { _modelNumber = num; }
    void setModelURL(const char *url)           { _modelURL = url; }
    void setSerialNumber(const char *num)       { _serialNumber = num; }
    void setURL(const char *url)                { _url = url; }
    void setManufacturer(const char *mf)        { _manufacturer = mf; }
    void setManufacturerURL(const char *url)    { _manufacturerURL = url; }
    void setName(const char *name)              { _name = name; }
    void setHTTPPort(uint16_t port)             { _httpPort = port; }

private:
    const char *_schemaURL      = "description.xml";
    const char *_deviceType     = "urn:schemas-upnp-org:device:Basic:1";
    const char *_modelName      = "";
    const char *_modelNumber    = "";
    const char *_modelURL       = "";
    const char *_serialNumber   = "";
    const char *_url            = "";
    const char *_manufacturer   = "";
    const char *_manufacturerURL= "";
    const char *_name           = "";
    uint16_t    _httpPort       = 80;
};

extern SSDPClass SSDP;

#endif // ESP8266SSDP_H
