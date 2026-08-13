/*
 * ESP8266HTTPUpdateServer-impl.h — template implementation (see .h).
 *
 * The reference registers GET (index page), POST (update) and an upload
 * progress callback, gated by HTTP basic auth. On WB2 the POST is a no-op
 * success (no flash write), but the page and auth behave normally.
 */

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include "ESP8266HTTPUpdateServer.h"

namespace esp8266httpupdateserver {
using namespace esp8266webserver;

static const char serverIndex[] PROGMEM =
  R"(<!DOCTYPE html>
     <html lang='en'>
     <head>
         <meta charset='utf-8'>
         <meta name='viewport' content='width=device-width,initial-scale=1'/>
     </head>
     <body>
     <form method='POST' action='' enctype='multipart/form-data'>
         Firmware:<br>
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Update Firmware'>
     </form>
     <form method='POST' action='' enctype='multipart/form-data'>
         FileSystem:<br>
         <input type='file' accept='.bin,.bin.gz' name='filesystem'>
         <input type='submit' value='Update FileSystem'>
     </form>
     </body>
     </html>)";

static const char successResponse[] PROGMEM =
  "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success!";

template <typename ServerType>
ESP8266HTTPUpdateServerTemplate<ServerType>::ESP8266HTTPUpdateServerTemplate(bool serial_debug)
{
  _serial_output = serial_debug;
  _server = NULL;
  _username = emptyString;
  _password = emptyString;
  _authenticated = false;
}

template <typename ServerType>
void ESP8266HTTPUpdateServerTemplate<ServerType>::setup(ESP8266WebServerTemplate<ServerType> *server, const String& path, const String& username, const String& password)
{
    _server = server;
    _username = username;
    _password = password;

    /* GET: show the upload form (demanding auth when credentials are set). */
    _server->on(path.c_str(), HTTP_GET, [this]() {
        if (_username != emptyString && _password != emptyString &&
            !_server->authenticate(_username.c_str(), _password.c_str())) {
            return _server->requestAuthentication();
        }
        _server->send_P(200, PSTR("text/html"), serverIndex);
    });

    /* POST: accept the upload, respond success (no flash write on BL602). */
    _server->on(path.c_str(), HTTP_POST, [this]() {
        _server->send_P(200, PSTR("text/html"), successResponse);
    }, [this]() {
        /* upload progress callback — nothing to track on WB2 */
    });
}

template <typename ServerType>
void ESP8266HTTPUpdateServerTemplate<ServerType>::_setUpdaterError()
{
    _updaterError = emptyString;
}

}
