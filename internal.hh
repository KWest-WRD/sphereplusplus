/**
 * @file internal.hh
 * @author Matthieu Bucchianeri
 * @brief Internal functions.
 */

#pragma once

#include <applibs/eventloop.h>

#define MJSON_ENABLE_PRINT  0
#define MJSON_ENABLE_RPC    0
#define MJSON_ENABLE_BASE64 0
#define MJSON_ENABLE_MERGE  0
#define MJSON_ENABLE_PRETTY 0
#define MJSON_ENABLE_NEXT   1
#include "mjson/src/mjson.h"

namespace SpherePlusPlus {

/**
 * @brief Get the application's event loop.
 * @return The event loop.
 * @note Requires the Application to be initialized.
 */
extern EventLoop *getEventLoop();

} /* namespace SpherePlusPlus */
