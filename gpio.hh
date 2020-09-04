/**
 * @file gpio.hh
 * @author Matthieu Bucchianeri
 * @brief General-Purpose Input/Output subsystem abstraction.
 */

#pragma once

#include <unistd.h>

#include <sphereplusplus/abort.hh>

#include <applibs/gpio.h>

namespace SpherePlusPlus {

/**
 * @brief Base GPIO class.
 * @see GpioIn, GpioOut
 */
class Gpio
{
public:
    /**
     * @brief Destructor.
     */
    virtual ~Gpio()
    {
        destroy();
    }

    /**
     * @brief Initialize the GPIO.
     * @return True on success.
     */
    virtual bool init()
    {
        AbortIf(m_gpioFd >= 0, false);

        return true;
    }

    /**
     * @brief Set the output level of the GPIO.
     * @param[in] level The level to output, true for high and false for low.
     * @return True on success.
     *
     * @note Fails when invoked on an input GPIO.
     */
    virtual bool set(const bool level)
    {
        AbortIfNot(m_gpioFd >= 0, false);
        AbortIfNot(m_isOutput, false);

        const GPIO_Value_Type value = level ? GPIO_Value_High : GPIO_Value_Low;
        AbortErrno(GPIO_SetValue(m_gpioFd, value), false);

        return true;
    }

    /**
     * @brief Get the state of the GPIO.
     * @param[out] level The state of the GPIO, true for high and false for low.
     * @return True on success.
     */
    virtual bool get(bool &level) const
    {
        AbortIfNot(m_gpioFd >= 0, false);

        GPIO_Value_Type value;
        AbortErrno(GPIO_GetValue(m_gpioFd, &value), false);
        level = value == GPIO_Value_High;

        return true;
    }

    /**
     * @brief Destroy the GPIO.
     * @return True on success.
     */
    virtual bool destroy()
    {
        AbortIfNot(m_gpioFd >= 0, false);

        AbortErrno(close(m_gpioFd), false);
        m_gpioFd = -1;

        return true;
    }

protected:
    /**
     * @brief Constructor.
     * @param[in] gpioId The GPIO unique identifier.
     * @param[in] isOutput Whether the GPIO is instantiated for output.
     */
    Gpio(const GPIO_Id gpioId, const bool isOutput) :
        m_gpioId(gpioId),
        m_gpioFd(-1),
        m_isOutput(isOutput)
    {
    }

    /**
     *  The GPIO unique identifier.
     */
    const GPIO_Id m_gpioId;

    /**
     * The underlying file descriptor of the GPIO.
     */
    int m_gpioFd;

private:
    /**
     * Whether the GPIO is instantiated for output.
     */
    const bool m_isOutput;
};

/**
 * @brief An input GPIO.
 * @see Gpio
 */
class GpioIn : public Gpio
{
public:
    /**
     * @brief Constructor.
     * @param[in] gpioId The GPIO unique identifier.
     */
    GpioIn(const GPIO_Id gpioId) :
        Gpio(gpioId, false)
    {
    }

    /**
     * @brief Initialize the GPIO.
     * @return True on success.
     */
    virtual bool init() override
    {
        AbortIfNot(Gpio::init(), false);

        m_gpioFd = GPIO_OpenAsInput(m_gpioId);
        AbortErrno(m_gpioFd, false);

        return true;
    }
};

/**
 * @brief An output GPIO.
 * @see Gpio
 */
class GpioOut : public Gpio
{
public:
    /**
     * @brief Constructor.
     * @param[in] gpioId The GPIO unique identifier.
     * @param[in] gpioOutputMode The GPIO mode (push-pull, open drain...).
     */
    GpioOut(const GPIO_Id gpioId, const GPIO_OutputMode gpioOutputMode) :
        Gpio(gpioId, true),
        m_gpioOutputMode(gpioOutputMode)
    {
    }

    /**
     * @brief Initialize the GPIO.
     * @param[in] level The initial level to output, true for high and false for
     *            low.
     * @return True on success.
     */
    virtual bool init(const bool level)
    {
        AbortIfNot(Gpio::init(), false);

        const GPIO_Value_Type value = level ? GPIO_Value_High : GPIO_Value_Low;

        m_gpioFd = GPIO_OpenAsOutput(m_gpioId, m_gpioOutputMode, value);
        AbortErrno(m_gpioFd, false);

        return true;
    }

private:
    /**
     * The GPIO mode (push-pull, open drain...).
     */
    const GPIO_OutputMode m_gpioOutputMode;
};

} /* namespace SpherePlusPlus */
