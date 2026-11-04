#pragma once

#include "http.hpp"
#include "utils/logger.h"

#include <string>

// Where figure dumps live on the SD card. Every path the web page sends is
// relative to this folder.
#define RE_NSYSHID_FIGURE_ROOT "/vol/external01/wiiu/re_nsyshid/"

// Decodes %XX escapes and '+' from a URL query value.
std::string urlDecode(const std::string &encoded);

// True when a path relative to RE_NSYSHID_FIGURE_ROOT cannot climb out of it.
bool isSafeRelativePath(const std::string &path);

void registerFileEndpoints(HttpServer &server);
