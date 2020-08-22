/**
 * @file sphere++.cc
 * @author Matthieu Bucchianeri
 * @brief Common symbols.
 */

#include <stddef.h>

#include <sphereplusplus/abort.hh>
#include <sphereplusplus/application.hh>

#include <applibs/eventloop.h>

#include "internal.hh"

extern EventLoop *getEventLoop();

/*
 * Define this symbol to keep the compiler happy with virtual destructors.
 */
void operator delete(void *p, size_t size)
{
    (void)p;
    (void)size;
}

EventLoop *getEventLoop()
{
    AbortIfNot(Application::g_application, nullptr);

    return Application::g_application->m_eventLoop;
}

Application *Application::g_application = nullptr;
