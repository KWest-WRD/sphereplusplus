/**
 * @file sphere++.cc
 * @author Matthieu Bucchianeri
 * @brief Common symbols.
 */

#include <stddef.h>

#include <sphereplusplus/application.hh>

/*
 * Define this symbol to keep the compiler happy with virtual destructors.
 */
void operator delete(void *p, size_t size)
{
    (void)p;
    (void)size;
}

Application *Application::g_application = nullptr;
