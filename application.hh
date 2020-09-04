/**
 * @file application.hh
 * @author Matthieu Bucchianeri
 * @brief Base application functionality.
 */

#pragma once

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <sphereplusplus/abort.hh>
#include <sphereplusplus/enums.hh>
#include <sphereplusplus/timer.hh>

#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <applibs/powermanagement.h>
#include <applibs/sysevent.h>

#include <azureiot/azure_sphere_provisioning.h>
#include <azureiot/iothub_client_core_common.h>
#include <azureiot/iothub_client_options.h>
#include <azureiot/iothub_device_client_ll.h>

namespace SpherePlusPlus {

/**
 * @brief Features supported by the application.
 */
enum class ApplicationFeatures : uint8_t
{
    /**
     * Enable notifications for updates.
     * @note Requires the "SystemEventNotifications" application capability and
     *       the "PowerControls" application capability to specify
     *       "ForceReboot".
     */
    UpdateNotification = 0x01,

    /**
     * Enable Time Synchronization with NTP.
     * @note Requires the "TimeSyncConfig" application capability.
     */
    TimeSync = 0x02,

    /**
     * Enable the watchdog.
     * @note Requires the "PowerControls" application capability to specify
     *       "ForceReboot".
     */
    Watchdog = 0x04,

    /**
     * Enable connection to Azure IoT Central.
     * @note Requires the proper "AllowedConnections" and "DeviceAuthentication"
     *       application capabilities.
     */
    IoTCentral = 0x08,

    /**
     * Enable periodic keepalive to Azure IoT Central.
     */
    Keepalive = 0x10,
};
ENABLE_BITMASK_OPERATORS(ApplicationFeatures);

/**
 * @brief Base application class, abstracting the event loop and basic
 *        functionality.
 */
class Application
{
public:
    /**
     * The default watchdog period, in seconds.
     */
    static constexpr uint32_t k_defaultWatchdogPeriod = 60;

    /**
     * The default maximum retry interval when connecting to Azure IoT Central,
     * in seconds.
     */
    static constexpr uint32_t k_defaultIotMaxRetryInterval = 120;

    /**
     * The initial retry interval when connecting to Azure IoT Central, in
     * seconds.
     */
    static constexpr uint32_t k_initialIotRetryInterval = 10;

    /**
     * The default keepalive period to Azure IoT Central, in seconds.
     */
    static constexpr uint32_t k_defaultKeepalivePeriod = 30;

    /**
     * @brief Constructor.
     */
    Application() :
        m_eventLoop(nullptr),
        m_running(false),
        m_sysevent(nullptr),
        m_iotConnectTimer(),
        m_iotHandle(nullptr),
        m_iotConnected(false),
        m_iotScopeId(),
        m_iotRetryInterval(k_initialIotRetryInterval),
        m_iotMaxRetryInterval(k_defaultIotMaxRetryInterval),
        m_useIot(false),
        m_keepalivePeriod(k_defaultKeepalivePeriod),
        m_useKeepalive(false),
        m_watchdogPeriod(k_defaultWatchdogPeriod),
        m_useWatchdog(false),
        m_oldTermAction(),
        m_oldAlrmAction()
    {
    }

    /**
     * @brief Destructor.
     */
    virtual ~Application()
    {
        destroy();
    }

    /**
     * @brief Initialize the application.
     * @param[in] features A bitmask of features to enable.
     * @return True on success.
     */
    virtual bool init(const ApplicationFeatures &features)
    {
        AbortIf(g_application, false);
        AbortIf(m_eventLoop, false);

        m_eventLoop = EventLoop_Create();
        AbortIfNot(m_eventLoop, false);

        g_application = this;

        /*
         * Register a handler for the termination signal.
         */
        struct sigaction term_action = {};
        term_action.sa_handler = signalHandler;
        AbortErrno(sigaction(SIGTERM, &term_action, &m_oldTermAction), false);

        /*
         * Initialize the requested features.
         */

        if (isSet(features, ApplicationFeatures::UpdateNotification)) {
            m_sysevent = SysEvent_RegisterForEventNotifications(
                m_eventLoop, SysEvent_Events_UpdateReadyForInstall,
                syseventCallback, this);
            AbortErrnoPtr(m_sysevent, false);
        }

        if (isSet(features, ApplicationFeatures::TimeSync)) {
            AbortErrno(Networking_TimeSync_SetEnabled(true), false);
        }

        m_useWatchdog = isSet(features, ApplicationFeatures::Watchdog);
        if (m_useWatchdog) {
            struct sigaction alrm_action = {};
            alrm_action.sa_handler = signalHandler;
            AbortErrno(sigaction(SIGALRM, &alrm_action, &m_oldAlrmAction),
                       false);

            AbortIfNot(petWatchdog(), false);
        }

        m_useIot = isSet(features, ApplicationFeatures::IoTCentral);
        m_useKeepalive = isSet(features, ApplicationFeatures::Keepalive);
        if (m_useIot) {
            AbortIfNot(m_iotConnectTimer.init(), false);

            m_iotConnectTimer.connect<
                Application, &Application::retryConnectIot>(*this);

            AbortIfNot(tryConnectIot(), false);
        }

        return true;
    }

    /**
     * @brief Initialize the application.
     * @param[in] features A bitmask of features to enable.
     * @param[in] watchdog_period_s The watchdog period, in seconds.
     * @return True on success.
     */
    virtual bool init(const ApplicationFeatures &features,
                      const uint32_t watchdog_period_s)
    {
        AbortIf(m_eventLoop, false);

        AbortIfNot(watchdog_period_s > 0, false);

        m_watchdogPeriod = watchdog_period_s;

        AbortIfNot(init(features), false);

        return true;
    }

    /**
     * @brief Initialize the application.
     * @param[in] features A bitmask of features to enable.
     * @param[in] azscope The Azure IoT Central scope ID.
     * @return True on success.
     */
    virtual bool init(const ApplicationFeatures &features,
                      const char *const azscope)
    {
        AbortIf(m_eventLoop, false);

        AbortIfNot(azscope, false);

        snprintf(m_iotScopeId, sizeof(m_iotScopeId), "%s", azscope);

        AbortIfNot(init(features), false);

        return true;
    }

    /**
     * @brief Initialize the application.
     * @param[in] features A bitmask of features to enable.
     * @param[in] azscope The Azure IoT Central scope ID.
     * @param[in] keepalive_period_s The Azure IoT keepalive period, in seconds.
     * @return True on success.
     */
    virtual bool init(const ApplicationFeatures &features,
                      const char *const azscope,
                      const uint32_t keepalive_period_s)
    {
        AbortIf(m_eventLoop, false);

        AbortIfNot(azscope, false);
        AbortIfNot(keepalive_period_s > 0, false);

        snprintf(m_iotScopeId, sizeof(m_iotScopeId), "%s", azscope);
        m_keepalivePeriod = keepalive_period_s;

        AbortIfNot(init(features), false);

        return true;
    }

    /**
     * @brief Initialize the application.
     * @param[in] features A bitmask of features to enable.
     * @param[in] watchdog_period_s The watchdog period, in seconds.
     * @param[in] azscope The Azure IoT Central scope ID.
     * @return True on success.
     */
    virtual bool init(const ApplicationFeatures &features,
                      const uint32_t watchdog_period_s,
                      const char *const azscope)
    {
        AbortIf(m_eventLoop, false);

        AbortIfNot(watchdog_period_s > 0, false);
        AbortIfNot(azscope, false);

        m_watchdogPeriod = watchdog_period_s;
        snprintf(m_iotScopeId, sizeof(m_iotScopeId), "%s", azscope);

        AbortIfNot(init(features), false);

        return true;
    }

    /**
     * @brief Initialize the application.
     * @param[in] features A bitmask of features to enable.
     * @param[in] watchdog_period_s The watchdog period, in seconds.
     * @param[in] azscope The Azure IoT Central scope ID.
     * @param[in] keepalive_period_s The Azure IoT keepalive period, in seconds.
     * @return True on success.
     */
    virtual bool init(const ApplicationFeatures &features,
                      const uint32_t watchdog_period_s,
                      const char *const azscope,
                      const uint32_t keepalive_period_s)
    {
        AbortIf(m_eventLoop, false);

        AbortIfNot(watchdog_period_s > 0, false);
        AbortIfNot(azscope, false);
        AbortIfNot(keepalive_period_s > 0, false);

        m_watchdogPeriod = watchdog_period_s;
        snprintf(m_iotScopeId, sizeof(m_iotScopeId), "%s", azscope);
        m_keepalivePeriod = keepalive_period_s;

        AbortIfNot(init(features), false);

        return true;
    }

    /**
     * @brief Run the application's event loop.
     * @return True on success.
     */
    virtual bool run()
    {
        AbortIfNot(m_eventLoop, false);

        m_running = true;
        while (m_running) {
            const EventLoop_Run_Result status =
                EventLoop_Run(m_eventLoop, -1, false);

            if (status == EventLoop_Run_Failed && errno != EINTR) {
                AbortErrno(-1, false);
            }
        }

        return true;
    }

    /**
     * @brief Destroy the application.
     * @return True on success.
     */
    virtual bool destroy()
    {
        AbortIfNot(m_eventLoop, false);

        m_running = false;
        AbortErrno(EventLoop_Stop(m_eventLoop), false);

        if (m_useIot) {
            AbortIfNot(m_iotConnectTimer.stop(), false);

            if (m_iotConnected) {
                IoTHubDeviceClient_LL_Destroy(m_iotHandle);
                m_iotHandle = nullptr;
            }
        }

        if (m_useWatchdog) {
            alarm(0);
            AbortErrno(sigaction(SIGALRM, &m_oldAlrmAction, nullptr), false);
        }

        if (m_sysevent) {
            AbortErrno(SysEvent_UnregisterForEventNotifications(m_sysevent),
                       false);
            m_sysevent = nullptr;
        }

        EventLoop_Close(m_eventLoop);
        m_eventLoop = nullptr;

        AbortErrno(sigaction(SIGTERM, &m_oldTermAction, nullptr), false);

        g_application = nullptr;

        return true;
    }

    /**
     * @brief Callback for notifications of a pending application update.
     * @param[in] max_deferral_m The maximum value that the callback can pass to
     *            blockUpdate()
     * @return True on success.
     */
    virtual bool notifyAppUpdatePending(const uint32_t max_deferral_m)
    {
        return true;
    }

    /**
     * @brief Callback for notifications of a pending system update.
     * @param[in] max_deferral_m The maximum value that the callback can pass to
     *            blockUpdate()
     * @return True on success.
     */
    virtual bool notifySystemUpdatePending(const uint32_t max_deferral_m)
    {
        return true;
    }

    /**
     * @brief Callback for notifications of a completed application update.
     * @return True on success.
     */
    virtual bool notifyAppUpdateCompleted()
    {
        AbortIfNot(systemReboot(), false);

        return true;
    }

    /**
     * @brief Block system and application updates.
     * @param[in] duration_m The duration to block updates for, in minutes.
     * @return True on success.
     * @see allowUpdate
     * @note Requires the "SoftwareUpdateDeferral" application capability.
     */
    virtual bool blockUpdate(const uint32_t duration_m) final
    {
        AbortIfNot(m_eventLoop, false);

        AbortErrno(SysEvent_DeferEvent(SysEvent_Events_UpdateReadyForInstall,
                                       duration_m),
                   false);

        return true;
    }

    /**
     * @brief Allow system and application updates.
     * @return True on success.
     * @see blockUpdate
     */
    virtual bool allowUpdate() final
    {
        AbortIfNot(m_eventLoop, false);

        AbortErrno(SysEvent_ResumeEvent(SysEvent_Events_UpdateReadyForInstall),
                   false);

        return true;
    }

    /**
     * @brief Reboot the system.
     * @return True on success.
     * @note Requires the "PowerControls" application capability to specify
     *       "ForceReboot".
     */
    virtual bool systemReboot() final
    {
        AbortErrno(PowerManagement_ForceSystemReboot(), false);

        return true;
    }

    /**
     * @brief Suspend the system.
     * @param[in] duration_s The duration to suspend for, in seconds.
     * @return True on success.
     * @note Requires the "PowerControls" application capability to specify
     *       "ForcePowerDown".
     */
    virtual bool systemSuspend(const uint32_t duration_s) final
    {
        AbortErrno(PowerManagement_ForceSystemPowerDown(duration_s), false);

        return true;
    }

    /**
     * @brief Pet the watchdog.
     * @return True on success.
     * @note The application must be initialized with the Watchdog feature.
     */
    virtual bool petWatchdog() final
    {
        AbortIfNot(m_eventLoop, false);
        AbortIfNot(m_useWatchdog, false);

        alarm(m_watchdogPeriod);

        return true;
    }

    /**
     * @brief Change the period of the watchdog.
     * @param[in] period_s The watchdog period, in seconds.
     * @return True on success.
     * @note The application must be initialized with the Watchdog feature.
     */
    virtual bool setWatchdogPeriod(const uint32_t period_s) final
    {
        AbortIfNot(m_eventLoop, false);
        AbortIfNot(m_useWatchdog, false);

        AbortIfNot(period_s > 0, false);

        m_watchdogPeriod = period_s;

        /*
         * Pet the watchdog immediately to start using the new period.
         */
        AbortIfNot(petWatchdog(), false);

        return true;
    }

    /**
     * @brief Change the maximum connection retry interval when connecting to
     *        Azure IoT Central.
     * @param[in] max_retry_interval_s The maximum connection retry interval, in
     *            seconds.
     * @return True on success.
     * @note The application must be initialized with the AzureIoT feature.
     */
    virtual bool setMaxRetryInterval(const uint32_t max_retry_interval_s) final
    {
        AbortIfNot(m_eventLoop, false);
        AbortIfNot(m_useIot, false);

        AbortIfNot(max_retry_interval_s > 0, false);

        m_iotMaxRetryInterval = max_retry_interval_s;

        if (m_iotConnected) {
            AbortIfNeq(IoTHubDeviceClient_LL_SetRetryPolicy(
                        m_iotHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF,
                        m_iotMaxRetryInterval),
                       IOTHUB_CLIENT_OK, false);
        } else {
            /*
             * If the application is try to reconnect but the new maximum retry
             * interval is lower than the previous one, restart the connection
             * timer to apply the new interval now.
             */
            if (m_iotRetryInterval > m_iotMaxRetryInterval) {
                m_iotRetryInterval = m_iotMaxRetryInterval;

                AbortIfNot(m_iotConnectTimer.startOneShot(
                            m_iotRetryInterval * 1000000),
                           false);
            }
        }

        return true;
    }

    /**
     * @brief Change the period of the keepalive to Azure IoT.
     * @param[in] keepalive_period_s The Azure IoT keepalive period, in seconds.
     * @return True on success.
     * @note The application must be initialized with the Keepalive feature.
     */
    virtual bool setKeepalivePeriod(const uint32_t period_s) final
    {
        AbortIfNot(m_eventLoop, false);
        AbortIfNot(m_useKeepalive, false);

        AbortIfNot(period_s > 0, false);

        m_keepalivePeriod = period_s;

        if (m_iotConnected) {
            const int keepalive_option = period_s;

            AbortIfNeq(IoTHubDeviceClient_LL_SetOption(
                        m_iotHandle, OPTION_KEEP_ALIVE, &keepalive_option),
                       IOTHUB_CLIENT_OK, false);
        }

        return true;
    }

private:
    /**
     * @brief Process signal callback.
     * @param[in] signo The signal number.
     */
    static void signalHandler(const int signo)
    {
        Assert(g_application);

        switch (signo) {
            case SIGTERM:
                Log_Debug("Termination signal received, shutting down...\n");
                g_application->destroy();
                break;

            case SIGALRM:
                Log_Debug("Watchdog timeout, rebooting...\n");
                g_application->systemReboot();
                break;

            default:
                break;
        }
    }

    /**
     * @brief System event callback.
     * @param[in] event The type of the event.
     * @param[in] state The new status of the event.
     * @param[in] info Additional information about the event.
     * @param[in] context The Application object.
     */
    static void syseventCallback(const SysEvent_Events event,
                                 const SysEvent_Status state,
                                 const SysEvent_Info *const info,
                                 void *const context)
    {
        Assert(event == SysEvent_Events_UpdateReadyForInstall);

        Application *const application = static_cast<Application *>(context);

        SysEvent_Info_UpdateData update;
        AbortErrno(SysEvent_Info_GetUpdateData(info, &update));
        const bool isSystemUpdate =
            update.update_type == SysEvent_UpdateType_System;

        switch (state) {
            case SysEvent_Status_Pending:
                if (isSystemUpdate) {
                    AbortIfNot(application->notifySystemUpdatePending(
                        update.max_deferral_time_in_minutes));
                } else {
                    AbortIfNot(application->notifyAppUpdatePending(
                        update.max_deferral_time_in_minutes));
                }
                break;

            case SysEvent_Status_Complete:
                if (!isSystemUpdate) {
                    AbortIfNot(application->notifyAppUpdateCompleted());
                }
                break;

            default:
                break;
        }
    }

    /**
     * @brief Try connecting to Azure IoT Central.
     * @return True when no unexpected error occurs. If the connection did not
     *         succeed, True is returned and a retry timer is started.
     */
    bool tryConnectIot()
    {
        const AZURE_SPHERE_PROV_RETURN_VALUE status =
            IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(
                m_iotScopeId, 10000, &m_iotHandle);

        if (status.result != AZURE_SPHERE_PROV_RESULT_OK) {
            const char *stage;
            const char *error;

            switch (status.result) {
                case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
                    stage = "Connection";
                    error = "One or more parameters were invalid.";
                    break;

                case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
                    stage = "Connection";
                    error = "Device could not be provisioned as network is not "
                            "ready.";
                    break;

                case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
                case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
                    stage = "Provisioning";
                    error = PROV_DEVICE_RESULTStrings(status.prov_device_error);
                    break;

                case AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR:
                    stage = "IoT Hub";
                    error = IOTHUB_CLIENT_RESULTStrings(
                        status.iothub_client_error);
                    break;

                default:
                    stage = "Connection";
                    error = "Unknown";
                    break;
            }

            Log_Debug("Failed to connect to Azure IoT Central:\n"
                      "  %s status: %s\n", stage, error);

            /*
             * Retry with exponential back-off.
             */
            AbortIfNot(m_iotConnectTimer.startOneShot(
                        m_iotRetryInterval * 1000000),
                       false);

            m_iotRetryInterval *= m_iotRetryInterval;
            if (m_iotRetryInterval > m_iotMaxRetryInterval) {
                m_iotRetryInterval = m_iotMaxRetryInterval;
            }

            return true;
        }

        /*
         * Apply options that require the handle to be valid first.
         */
        if (m_useKeepalive) {
            const int keepalive_option = m_keepalivePeriod;

            AbortIfNeq(IoTHubDeviceClient_LL_SetOption(
                        m_iotHandle, OPTION_KEEP_ALIVE, &keepalive_option),
                       IOTHUB_CLIENT_OK, false);
        }

        AbortIfNeq(IoTHubDeviceClient_LL_SetRetryPolicy(
                    m_iotHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF,
                    m_iotMaxRetryInterval),
                   IOTHUB_CLIENT_OK, false);

        /*
         * Register all required callbacks.
         */
        AbortIfNeq(IoTHubDeviceClient_LL_SetConnectionStatusCallback(
                    m_iotHandle, iotConnectionCallback, this),
                   IOTHUB_CLIENT_OK, false);

        Log_Debug("Connected to Azure IoT Central\n");

        return true;
    }

    /**
     * @brief IoT Central reconnection timer callback.
     */
    void retryConnectIot()
    {
        AbortIfNot(tryConnectIot());
    }

    /**
     * @brief IoT Central connection callback.
     * @param[in] status The status of the connection.
     * @param[in] reason The reason for the reported status.
     * @param[in] context The Application object.
     */
    static void iotConnectionCallback(
        const IOTHUB_CLIENT_CONNECTION_STATUS status,
        const IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
        void *const context)
    {
        Application *const application = static_cast<Application *>(context);

        application->m_iotConnected =
            status == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED;
        if (!application->m_iotConnected) {
            Log_Debug("Failed to communicate with Azure IoT Central: %s\n",
                      IOTHUB_CLIENT_CONNECTION_STATUS_REASONStrings(reason));
        }
    }

    /**
     * The event loop.
     */
    EventLoop *m_eventLoop;

    /**
     *  Whether to keep the event loop running.
     */
    bool m_running;

    /**
     * The system event handler.
     */
    EventRegistration *m_sysevent;

    /**
     * The (re)connection timer to Azure IoT Central.
     */
    Timer m_iotConnectTimer;

    /**
     * The Azure IoT Central connection.
     */
    IOTHUB_DEVICE_CLIENT_LL_HANDLE m_iotHandle;

    /**
     * Whether the application is connected to Azure IoT Central.
     */
    bool m_iotConnected;

    /**
     * The Azure IoT Central scope ID.
     */
    char m_iotScopeId[64];

    /**
     * The current retry interval when connecting to Azure IoT Central, in
     * seconds.
     */
    uint32_t m_iotRetryInterval;

    /**
     * The maximum retry interval when connecting to Azure IoT Central, in
     * seconds.
     */
    uint32_t m_iotMaxRetryInterval;

    /**
     * Whether Azure IoT Central is used by the application.
     */
    bool m_useIot;

    /**
     * The Azure IoT Central keepalive period, in seconds.
     */
    uint32_t m_keepalivePeriod;

    /**
     * Whether to send keepalive to Azure IoT Central.
     */
    bool m_useKeepalive;

    /**
     * The watchdog period, in seconds.
     */
    uint32_t m_watchdogPeriod;

    /**
     * Whether the watchdog is used by the application.
     */
    bool m_useWatchdog;

    /**
     * Saved state for the signal handlers.
     * @{
     */
    struct sigaction m_oldTermAction;
    struct sigaction m_oldAlrmAction;
    /**
     * @}
     */

    /**
     * A pointer to the currently running application, used to handle process
     * signals.
     */
    static Application *g_application;

    friend EventLoop *getEventLoop();
};

} /* namespace SpherePlusPlus */
