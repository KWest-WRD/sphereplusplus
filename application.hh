/**
 * @file application.hh
 * @author Matthieu Bucchianeri
 * @brief Base application functionality.
 */

#pragma once

#include <errno.h>

#include <sphereplusplus/abort.hh>

#include <applibs/eventloop.h>

/**
 * @brief Base application class, abstracting the event loop and basic
 *        functionality.
 */
class Application
{
public:
    /*
     * @brief Constructor.
     */
    Application() :
        m_eventLoop(nullptr),
        m_running(false)
    {
    }

    /*
     * @brief Destructor.
     */
    virtual ~Application()
    {
        destroy();
    }

    /**
     * @brief Initialize the application.
     * @return True on success.
     */
    virtual bool init()
    {
        AbortIf(m_eventLoop, false);

        m_eventLoop = EventLoop_Create();
        AbortIfNot(m_eventLoop, false);

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
        EventLoop_Close(m_eventLoop);
        m_eventLoop = nullptr;

        return true;
    }

private:
    /**
     * The event loop.
     */
    EventLoop *m_eventLoop;

    /**
     *  Whether to keep the event loop running.
     */
    bool m_running;

    friend class Timer;
};
