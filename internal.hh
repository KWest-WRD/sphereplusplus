/**
 * @file internal.hh
 * @author Matthieu Bucchianeri
 * @brief Internal functions.
 */

#pragma once

#include <applibs/eventloop.h>

namespace SpherePlusPlus {

/**
 * @brief Get the application's event loop.
 * @return The event loop.
 * @note Requires the Application to be initialized.
 */
extern EventLoop *getEventLoop();

} /* namespace SpherePlusPlus */
