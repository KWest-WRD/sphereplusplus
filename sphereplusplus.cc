/**
 * @file sphereplusplus.cc
 * @author Matthieu Bucchianeri
 * @brief Common symbols.
 */

#include <stddef.h>

#include <sphereplusplus/abort.hh>
#include <sphereplusplus/application.hh>

#include <applibs/eventloop.h>

#include "internal.hh"

/*
 * Define this symbol to keep the compiler happy with virtual destructors.
 */
void operator delete(void *p, size_t size)
{
    (void)p;
    (void)size;
}

/*
 * This definition is somehow missing from the Azure IoT library on Sphere.
 */
const char *PROV_DEVICE_RESULTStrings(const PROV_DEVICE_RESULT result)
{
    switch (result) {
        case PROV_DEVICE_RESULT_OK:
            return "PROV_DEVICE_RESULT_OK";

        case PROV_DEVICE_RESULT_INVALID_ARG:
            return "PROV_DEVICE_RESULT_INVALID_ARG";

        case PROV_DEVICE_RESULT_SUCCESS:
            return "PROV_DEVICE_RESULT_SUCCESS";

        case PROV_DEVICE_RESULT_MEMORY:
            return "PROV_DEVICE_RESULT_MEMORY";

        case PROV_DEVICE_RESULT_PARSING:
            return "PROV_DEVICE_RESULT_PARSING";

        case PROV_DEVICE_RESULT_TRANSPORT:
            return "PROV_DEVICE_RESULT_TRANSPORT";

        case PROV_DEVICE_RESULT_INVALID_STATE:
            return "PROV_DEVICE_RESULT_INVALID_STATE";

        case PROV_DEVICE_RESULT_DEV_AUTH_ERROR:
            return "PROV_DEVICE_RESULT_DEV_AUTH_ERROR";

        case PROV_DEVICE_RESULT_TIMEOUT:
            return "PROV_DEVICE_RESULT_TIMEOUT";

        case PROV_DEVICE_RESULT_KEY_ERROR:
            return "PROV_DEVICE_RESULT_KEY_ERROR";

        case PROV_DEVICE_RESULT_ERROR:
            return "PROV_DEVICE_RESULT_ERROR";

        case PROV_DEVICE_RESULT_HUB_NOT_SPECIFIED:
            return "PROV_DEVICE_RESULT_HUB_NOT_SPECIFIED";

        case PROV_DEVICE_RESULT_UNAUTHORIZED:
            return "PROV_DEVICE_RESULT_UNAUTHORIZED";

        case PROV_DEVICE_RESULT_DISABLED:
            return "PROV_DEVICE_RESULT_DISABLED";

        default:
            return "Unknown";
    }
}

/*
 * This definition is somehow missing from the Azure IoT library on Sphere.
 */
const char *IOTHUB_CLIENT_RESULTStrings(const IOTHUB_CLIENT_RESULT result)
{
    switch (result) {
        case IOTHUB_CLIENT_OK:
            return "IOTHUB_CLIENT_OK";

        case IOTHUB_CLIENT_INVALID_ARG:
            return "IOTHUB_CLIENT_INVALID_ARG";

        case IOTHUB_CLIENT_ERROR:
            return "IOTHUB_CLIENT_ERROR";

        case IOTHUB_CLIENT_INVALID_SIZE:
            return "IOTHUB_CLIENT_INVALID_SIZE";

        case IOTHUB_CLIENT_INDEFINITE_TIME:
            return "IOTHUB_CLIENT_INDEFINITE_TIME";

        default:
            return "Unknown";
    }
}

/*
 * This definition is somehow missing from the Azure IoT library on Sphere.
 */
const char *IOTHUB_CLIENT_CONNECTION_STATUS_REASONStrings(
    const IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    switch (reason) {
        case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
            return "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";

        case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
            return "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";

        case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
            return "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";

        case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
            return "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";

        case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
            return "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";

        case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
            return "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";

        case IOTHUB_CLIENT_CONNECTION_OK:
            return "IOTHUB_CLIENT_CONNECTION_OK";

        case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
            return "IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE";

        default:
            return "Unknown";
    }
}

namespace SpherePlusPlus {

EventLoop *getEventLoop()
{
    AbortIfNot(Application::g_application, nullptr);

    return Application::g_application->m_eventLoop;
}

Application *Application::g_application = nullptr;

} /* namespace SpherePlusPlus */

/*
 * Include the mjson parser as configured in internal.hh.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "mjson/src/mjson.c"
#pragma GCC diagnostic pop
