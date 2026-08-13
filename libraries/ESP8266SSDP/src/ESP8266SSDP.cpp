/*
 * ESP8266SSDP.cpp - minimal device-description serving; see ESP8266SSDP.h.
 */

#include "ESP8266SSDP.h"

#include <WiFiClient.h>
#include <ESP8266WiFi.h>

SSDPClass SSDP;

bool SSDPClass::schema(WiFiClient &client)
{
    char buf[256];
    client.print("HTTP/1.1 200 OK\r\n");
    client.print("Content-Type: text/xml\r\n");
    client.print("Connection: close\r\n\r\n");
    snprintf(buf, sizeof(buf),
             "<?xml version=\"1.0\"?>"
             "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
             "<specVersion><major>1</major><minor>0</minor></specVersion>"
             "<URLBase>http://%s:%u/</URLBase>"
             "<device><deviceType>%s</deviceType>"
             "<friendlyName>%s</friendlyName>"
             "<manufacturer>%s</manufacturer>"
             "<manufacturerURL>%s</manufacturerURL>"
             "<modelDescription>%s</modelDescription>"
             "<modelName>%s</modelName>"
             "<modelNumber>%s</modelNumber>"
             "<modelURL>%s</modelURL>"
             "<serialNumber>%s</serialNumber>"
             "<UDN>uuid:%s</UDN>"
             "</device></root>",
             WiFi.localIP().toString().c_str(), _httpPort,
             _deviceType, _name, _manufacturer, _manufacturerURL,
             _modelName, _modelName, _modelNumber, _modelURL, _serialNumber,
             _serialNumber);
    client.print(buf);
    return true;
}
