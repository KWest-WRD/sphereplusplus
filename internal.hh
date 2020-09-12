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

// XXX: doxy
class Value
{
public:
    virtual bool isUpdated(const bool peek = false)
    {
        const bool updated = m_updated;
        if (!peek) {
            m_updated = false;
        }
        return updated;
    }

    virtual bool isDirty()
    {
        return m_dirty;
    }

protected:
    Value(const char *const name) :
        m_name(name),
        m_updated(false),
        m_dirty(false)
    {
    }

    virtual ~Value()
    {
    }

    virtual void addToDocument(/* XXX JSON */)
    {
    }

    const char *const m_name;

    bool m_updated;
    bool m_dirty;
};

// XXX: doxy
template <typename T>
class TypedValue : public Value
{
public:
    virtual T get() const
    {
        return m_value;
    }

    virtual T set(const T value)
    {
        m_value = value;
        return value;
    }

protected:
    TypedValue(const char *const name) :
        Value(name)
    {
    }

    virtual ~TypedValue()
    {
    }

    virtual void addToDocument()
    {
    }

    T m_value;
};

/**
 * @brief Get the application's event loop.
 * @return The event loop.
 * @note Requires the Application to be initialized.
 */
extern EventLoop *getEventLoop();

} /* namespace SpherePlusPlus */
