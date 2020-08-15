/**
 * @file application.hh
 * @author Matthieu Bucchianeri
 * @brief Base application functionality.
 */

#pragma once

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include <sphereplusplus/abort.hh>
#include <sphereplusplus/enums.hh>

#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <applibs/powermanagement.h>
#include <applibs/sysevent.h>

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
     * @brief Constructor.
     */
    Application() :
        m_eventLoop(nullptr),
        m_running(false),
        m_sysevent(nullptr),
        m_watchdogPeriod(60),
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

        AbortIfNot(setWatchdogPeriod(watchdog_period_s), false);

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
     */
    virtual bool petWatchdog() final
    {
        alarm(m_watchdogPeriod);

        return true;
    }

    /**
     * @brief Change the period of the watchdog.
     * @param[in] watchdog_period_s The watchdog period, in seconds.
     * @return True on success.
     */
    virtual bool setWatchdogPeriod(const uint32_t period_s) final
    {
        AbortIfNot(period_s > 0, false);

        m_watchdogPeriod = period_s;

        /*
         * If the application is initialized, pet the watchdog immediately to
         * start using the new period.
         */
        if (m_eventLoop) {
            AbortIfNot(petWatchdog(), false);
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

    friend class Timer;
};
