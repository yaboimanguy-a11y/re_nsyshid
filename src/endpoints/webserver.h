#pragma once

#include <string>

// The port the web page and the JSON API are served on.
#define RE_NSYSHID_WEB_PORT 8853

#define ENABLE_WEB_SERVER_CONFIG_ID     "enableServer"
#define ENABLE_WEB_SERVER_DEFAULT_VALUE false

// Starts the HTTP server on its own thread. Safe to call when it is already running.
void startWebServer();
// Stops the HTTP server. Safe to call when it is not running.
void stopWebServer();
bool isWebServerRunning();

// One line for the plugin menu: listening + request count, or why it stopped.
std::string webServerStatus();

// "http://192.168.1.20:8853" when the console has an address, empty otherwise.
std::string webServerAddress();
