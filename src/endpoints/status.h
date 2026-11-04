#pragma once

#include "http.hpp"
#include "utils/logger.h"
#include <wups.h>

// http.hpp can pull in <cmath>, whose INFINITY macro would clobber the
// DeviceToEmulate::INFINITY enumerator declared in re_nsyshid.h.
#ifdef INFINITY
#undef INFINITY
#endif
#include "re_nsyshid.h"

void registerStatusEndpoints(HttpServer &server);
