/**
 * @file sphere++.cc
 * @author Matthieu Bucchianeri
 * @brief Common symbols.
 */

#include <stddef.h>

/*
 * Define this symbol to keep the compiler happy with virtual destructors.
 */
void operator delete(void *p, size_t size)
{
    (void)p;
    (void)size;
}
