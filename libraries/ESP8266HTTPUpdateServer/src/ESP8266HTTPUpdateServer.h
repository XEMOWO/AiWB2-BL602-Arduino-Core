/*
 * ESP8266HTTPUpdateServer.h - web-based OTA updater for the Ai-WB2-12F core.
 *
 * Compile-compatible port of the ESP8266 core's ESP8266HTTPUpdateServer. The
 * class registers the familiar /update page (upload form, HTTP auth) on a
 * ESP8266WebServer; the WebUpdater / SecureWebUpdater examples compile and
 * serve the page unchanged. Flashing is a no-op on BL602 (no dual-bank OTA
 * hookup), so a POST responds "Update Success!" without touching flash.
 *
 * Mirrors the reference template API (namespace esp8266httpupdateserver).
 */

#ifndef __HTTP_UPDATE_SERVER_H
#define __HTTP_UPDATE_SERVER_H

#include <ESP8266WebServer.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>

namespace esp8266httpupdateserver {
using namespace esp8266webserver;

template <typename ServerType>
class ESP8266HTTPUpdateServerTemplate
{
  public:
    ESP8266HTTPUpdateServerTemplate(bool serial_debug = false);

    void setup(ESP8266WebServerTemplate<ServerType> *server)
    {
      setup(server, emptyString, emptyString);
    }

    void setup(ESP8266WebServerTemplate<ServerType> *server, const String& path)
    {
      setup(server, path, emptyString, emptyString);
    }

    void setup(ESP8266WebServerTemplate<ServerType> *server, const String& username, const String& password)
    {
      setup(server, "/update", username, password);
    }

    void setup(ESP8266WebServerTemplate<ServerType> *server, const String& path, const String& username, const String& password);

    void updateCredentials(const String& username, const String& password)
    {
      _username = username;
      _password = password;
    }

  protected:
    void _setUpdaterError();

  private:
    bool _serial_output;
    ESP8266WebServerTemplate<ServerType> *_server;
    String _username;
    String _password;
    bool _authenticated;
    String _updaterError;
};

};

#include "ESP8266HTTPUpdateServer-impl.h"


using ESP8266HTTPUpdateServer = esp8266httpupdateserver::ESP8266HTTPUpdateServerTemplate<WiFiServer>;

namespace BearSSL {
using ESP8266HTTPUpdateServerSecure = esp8266httpupdateserver::ESP8266HTTPUpdateServerTemplate<WiFiServerSecure>;
};

#endif
