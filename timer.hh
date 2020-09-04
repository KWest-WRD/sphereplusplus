/**
 * @file timer.hh
 * @author Matthieu Bucchianeri
 * @brief One shot or periodic timers.
 */

#pragma once

#include <sys/timerfd.h>
#include <stdint.h>
#include <unistd.h>

#include <applibs/eventloop.h>

#include <sphereplusplus/abort.hh>
#include <sphereplusplus/delegate.hh>

#include "internal.hh"

namespace SpherePlusPlus {

/**
 * @brief One shot or periodic timers.
 */
class Timer
{
public:
    /**
     * @brief Constructor.
     */
    Timer() :
        m_callback(),
        m_timerFd(-1),
        m_event(nullptr)
    {
    }

    /**
     * @brief Destructor.
     */
    virtual ~Timer()
    {
        destroy();
    }

    /**
     * @brief Initialize the timer.
     * @return True on success.
     */
    virtual bool init()
    {
        AbortIf(m_timerFd >= 0, false);

        m_timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        AbortErrno(m_timerFd, false);

        m_event = EventLoop_RegisterIo(getEventLoop(), m_timerFd,
                                       EventLoop_Input, callback, this);
        AbortErrnoPtr(m_event, false);

        return true;
    }

    /**
     * @brief Destroy the timer.
     * @return True on success.
     */
    virtual bool destroy()
    {
        AbortIfNot(m_timerFd >= 0, false);

        AbortErrno(EventLoop_UnregisterIo(getEventLoop(), m_event), false);
        m_event = nullptr;

        AbortErrno(close(m_timerFd), false);
        m_timerFd = -1;

        return true;
    }

    /**
     * @brief Connect a class method to the timer's expiry.
     * @tparam T The class type.
     * @tparam TMethod The class method.
     * @param[in] instance The class instance.
     */
    template<class T, void (T::*TMethod)()>
    void connect(T &instance)
    {
        m_callback.connect<T, TMethod>(instance);
    }

    /**
     * @brief Connect a const class method to the timer's expiry.
     * @tparam T The class type.
     * @tparam TMethod The class method.
     * @param[in] instance The class instance.
     */
    template<class T, void (T::*TMethod)() const>
    void connect(T &instance)
    {
        m_callback.connect<T, TMethod>(instance);
    }

    /**
     * @brief Connect a static method to the timer's expiry.
     * @tparam TFunc The static method.
     */
    template<void (*TFunc)()>
    void connect()
    {
        m_callback.connect<TFunc>();
    }

    /**
     * @brief Connect a lambda to the timer's expiry.
     * @tparam LAMBDA The lambda type.
     * @param[in] instance The closure for the lambda.
     */
    template <typename LAMBDA>
    void connect(const LAMBDA &instance)
    {
        m_callback.connect<LAMBDA>(instance);
    }

    /**
     * @brief Start the timer in one-shot mode.
     * @param[in] delay_us The delay before the shot, in microseconds.
     * @return True on success.
     */
    virtual bool startOneShot(const uint64_t delay_us)
    {
        AbortIfNot(m_timerFd >= 0, false);

        const struct itimerspec oneShot = {
            .it_interval = {},
            .it_value = makeTimespec(delay_us),
        };

        AbortErrno(timerfd_settime(m_timerFd, 0, &oneShot, nullptr), false);

        return true;
    }

    /**
     * @brief Start the timer in periodic mode.
     * @param[in] period_us The period of the timer, in microseconds.
     * @return True on success.
     */
    virtual bool startPeriodic(const uint64_t period_us)
    {
        AbortIfNot(m_timerFd >= 0, false);

        const struct timespec period = makeTimespec(period_us);
        const struct itimerspec periodic = {
            .it_interval = period,
            .it_value = period,
        };

        AbortErrno(timerfd_settime(m_timerFd, 0, &periodic, nullptr), false);

        return true;
    }

    /**
     * @brief Stop the timer.
     * @return True on success.
     */
    virtual bool stop()
    {
        static constexpr struct itimerspec stop = {
            .it_interval = {},
            .it_value = {},
        };

        AbortIfNot(m_timerFd >= 0, false);

        AbortErrno(timerfd_settime(m_timerFd, 0, &stop, nullptr), false);

        return true;
    }

private:

    /**
     * @brief Helper function to make a timespec from a given time.
     * @param[in] micros The time to convert, in microseconds.
     * @return A timespec representing the specified time.
     */
    static struct timespec makeTimespec(const uint64_t micros)
    {
        const struct timespec spec = {
            .tv_sec = static_cast<time_t>(micros / 1000000),
            .tv_nsec = static_cast<long>((micros % 1000000) * 1000),
        };

        return spec;
    }

    /**
     * @brief Timer callback. Invokes the user callback.
     * @param[in] el The event loop.
     * @param[in] fd The file descriptor that triggered the event.
     * @param[in] events The type of the event.
     * @param[in] context The Timer object.
     */
    static void callback(EventLoop *const el, const int fd,
                         const EventLoop_IoEvents events, void *const context)
    {
        Assert(events == EventLoop_Input);

        Timer *const timer = static_cast<Timer *>(context);
        Assert(fd == timer->m_timerFd);

        /*
         * Read the event, but no need to do anything with its payload.
         */
        uint64_t payload;
        const ssize_t count = read(timer->m_timerFd, &payload, sizeof(payload));
        AbortIfNot(count == sizeof(payload));

        timer->m_callback();
    }

    /**
     * The timer's user callback.
     */
    Delegate<void()> m_callback;

    /**
     * The underlying file descriptor of the timer.
     */
    int m_timerFd;

    /**
     * The event handler.
     */
    EventRegistration *m_event;
};

} /* namespace SpherePlusPlus */
