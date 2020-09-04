/**
 * @file delegate.hh
 * @author Matthieu Bucchianeri
 * @brief Fast delegate.
 *
 * Largely inspired by:
 * https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed
 */

#pragma once

#include <sphereplusplus/abort.hh>

namespace SpherePlusPlus {

template<typename T>
class Delegate;

/**
 * @brief Delegate callbacks.
 * @tparam RET The return type of the callback.
 * @tparam PARAMS The arguments of the callback.
 */
template<typename RET, typename ...PARAMS>
class Delegate<RET(PARAMS...)>
{
public:
    /**
     * @brief Constructor.
     */
    Delegate() :
        m_object(nullptr),
        m_stub(nullptr)
    {
    }

    /**
     * @brief Copy constructor.
     * @param[in] other The instance to copy.
     */
    Delegate(const Delegate &other) :
        m_object(other.m_object),
        m_stub(other.m_stub)
    {
    }

    /**
     * @brief Assignment operator.
     * @param[in] other The instance to copy.
     * @return This instance.
     */
    Delegate &operator =(const Delegate &other)
    {
        m_object = other.m_object;
        m_stub = other.m_stub;

        return *this;
    }

    /**
     * @brief Connect a class method to the delegate.
     * @tparam T The class type.
     * @tparam TMethod The class method.
     * @param[in] instance The class instance.
     */
    template <class T, RET (T::*TMethod)(PARAMS...)>
    void connect(T &instance)
    {
        m_object = static_cast<void *>(&instance);
        m_stub = method_stub<T, TMethod>;
    }

    /**
     * @brief Connect a const class method to the delegate.
     * @tparam T The class type.
     * @tparam TMethod The class method.
     * @param[in] instance The class instance.
     */
    template <class T, RET (T::*TMethod)(PARAMS...) const>
    void connect(T &instance)
    {
        m_object = static_cast<void *>(&instance);
        m_stub = const_method_stub<T, TMethod>;
    }

    /**
     * @brief Connect a static method to the delegate.
     * @tparam TFunc The static method.
     */
    template <RET (*TFunc)(PARAMS...)>
    void connect()
    {
        m_object = nullptr;
        m_stub = function_stub<TFunc>;
    }

    /**
     * @brief Connect a lambda to the delegate.
     * @tparam LAMBDA The lambda type.
     * @param[in] instance The closure for the lambda.
     */
    template <typename LAMBDA>
    void connect(const LAMBDA &instance)
    {
        m_object = (void *)&instance,
        m_stub = lambda_stub<LAMBDA>;
    }

    /**
     * @brief Invoke the callback.
     * @param arg User-specified arguments.
     * @return User-specified return value.
     */
    RET operator()(PARAMS... arg) const
    {
        Assert(m_stub != nullptr);

        return (*m_stub)(m_object, arg...);
    }

private:
    /**
     * @brief Type of the internal callback stub.
     */
    using stub_type = RET(*)(void *, PARAMS...);

    /**
     * @brief Forward a class method call.
     * @tparam T The class type.
     * @tparam TMethod The class method.
     * @param[in] this_ptr The class instance.
     * @param     params User-specified arguments.
     * @return User-specified return value.
     */
    template <class T, RET(T::*TMethod)(PARAMS...)>
    static RET method_stub(void *this_ptr, PARAMS... params)
    {
        T *p = static_cast<T *>(this_ptr);
        return (p->*TMethod)(params...);
    }

    /**
     * @brief Forward a const class method call.
     * @tparam T The class type.
     * @tparam TMethod The class method.
     * @param[in] this_ptr The class instance.
     * @param     params User-specified arguments.
     * @return User-specified return value.
     */
    template <class T, RET(T::*TMethod)(PARAMS...) const>
    static RET const_method_stub(void *this_ptr, PARAMS... params)
    {
        T *const p = static_cast<T *>(this_ptr);
        return (p->*TMethod)(params...);
    }

    /**
     * @brief Forward a static method call.
     * @tparam TFunc The static method.
     * @param[in] this_ptr Placeholder for the class instance (nullptr).
     * @param     params User-specified arguments.
     * @return User-specified return value.
     */
    template <RET(*TFunc)(PARAMS...)>
    static RET function_stub(void *this_ptr, PARAMS... params)
    {
        return (TFunc)(params...);
    }

    /**
     * @brief Forward a lambda call.
     * @tparam LAMBDA The lambda type.
     * @param[in] this_ptr The closure for the lambda.
     * @param     params User-specified arguments.
     * @return User-specified return value.
     */
    template <typename LAMBDA>
    static RET lambda_stub(void *this_ptr, PARAMS... arg)
    {
        LAMBDA *p = static_cast<LAMBDA *>(this_ptr);
        return (p->operator())(arg...);
    }

    /**
     * The instance/closure for the callback.
     */
    void *m_object;

    /**
     *  The internal callback stub.
     */
    stub_type m_stub;
};

} /* namespace SpherePlusPlus */
