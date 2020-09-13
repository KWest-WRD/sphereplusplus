/**
 * @file vector.hh
 * @author Matthieu Bucchianeri
 * @brief Simple dynamic vector implementation. Intended to look and feel like
 *        std::vector. Not thread-safe.
 */

#pragma once

#include <sys/types.h>

#include <sphereplusplus/abort.hh>

namespace SpherePlusPlus {

/**
 * @brief A vector of elements.
 * @tparam T The type of the elements.
 */
template<typename T>
class vector
{
private:
    /*
     * The default initial capacity of the vector.
     */
    static constexpr size_t k_defaultInitialCapacity = 20;

public:
    /**
     * @brief Constructor.
     * @param[in] initialCapacity The initial capacity of the vector.
     * @param[in] fixedCapacity Whether the vector can be dynamically resized
     *            when needed.
     */
    vector(const size_t initialCapacity = k_defaultInitialCapacity,
           const bool fixedCapacity = false) :
        m_array(nullptr),
        m_unmanagedArray(false),
        m_capacity(initialCapacity),
        m_fixedCapacity(fixedCapacity),
        m_count(0),
        m_first(),
        m_last()
    {
    }

    /**
     * @brief Constructor.
     * @param[in] buffer Provide a storage buffer for the vector.
     * @param[in] size The size of the storage buffer.
     * @note Vectors constructed with this constructor do not need to be
     *       initialized with init().
     * @note Vectors constructed with this constructor are fixed in size (cannot
     *       be dynamically resized).
     */
    vector(void *const buffer, const size_t size) :
        m_array((T *)buffer),
        m_unmanagedArray(true),
        m_capacity(size / sizeof(T)),
        m_fixedCapacity(true),
        m_count(0),
        m_first(),
        m_last()
    {
    }

    /**
     * @brief Destructor.
     */
    ~vector()
    {
        destroy();
    }

    /**
     * @brief Initialize the vector.
     * @return True on success.
     */
    bool init()
    {
        if (!m_unmanagedArray) {
            AbortIf(m_array != nullptr, false);
        }

        AbortIf(m_capacity == 0, false);

        if (!m_unmanagedArray) {
            m_array = static_cast<T *>(malloc(m_capacity * sizeof(T)));
            AbortErrnoPtr(m_array, false);
        }

        AbortIfNot(clear(), false);

        return true;
    }

    /**
     * @brief Destroy the vector.
     * @return True on success.
     */
    bool destroy()
    {
        AbortIfNot(m_array != nullptr, false);

        AbortIfNot(clear(), false);

        if (!m_unmanagedArray) {
            free(m_array);
            m_array = nullptr;
        }

        return true;
    }

    /**
     * @brief Clear the contents of the vector.
     * @return True on success.
     */
    bool clear()
    {
        AbortIfNot(m_array != nullptr, false);

        m_first = m_last = m_count = 0;

        return true;
    }

    /**
     * @brief Retrieve the element at the front (the first element) of the
     *        vector.
     * @param[out] elem On success, set to the first element of the vector.
     * @return True on success.
     */
    bool front(T &elem) const
    {
        AbortIfNot(m_array != nullptr, false);

        if (empty()) {
            return false;
        }

        elem = m_array[m_first];

        return true;
    }

    /**
     * @brief Retrieve the element at the back (the last element) of the
     *        vector.
     * @param[out] elem On success, set to the last element of the vector.
     * @return True on success.
     */
    bool back(T &elem) const
    {
        AbortIfNot(m_array != nullptr, false);

        if (empty()) {
            return false;
        }

        elem = m_array[m_last];

        return true;
    }

    /**
     * @brief Retrieve the element at given position of the vector.
     * @param[in] position The position in the vector.
     * @param[out] elem On success, set to the element from the vector.
     * @return True on success.
     */
    bool at(const size_t position, T &elem) const
    {
        AbortIfNot(m_array != nullptr, false);

        if (position >= size()) {
            return false;
        }

        const size_t offset = (m_first + position) % m_capacity;
        elem = m_array[offset];

        return true;
    }

    /**
     * @brief Prepend an element to the vector (at the front of the vector).
     * @param[in] elem The element to add to the vector.
     * @return True on success.
     */
    bool push_front(const T &elem)
    {
        AbortIfNot(m_array != nullptr, false);

        if (full()) {
            if (!m_fixedCapacity) {
                AbortIfNot(grow(), false);
            } else {
                return false;
            }
        }

        if (!empty()) {
            if (m_first) {
                m_first--;
            } else {
                m_first = m_capacity - 1;
            }
        }
        m_array[m_first] = elem;
        m_count++;

        return true;
    }

    /**
     * @brief Append an element to the vector (at the back of the vector).
     * @param[in] elem The element to add to the vector.
     * @return True on success.
     */
    bool push_back(const T &elem)
    {
        AbortIfNot(m_array != nullptr, false);

        if (full()) {
            if (!m_fixedCapacity) {
                AbortIfNot(grow(), false);
            } else {
                return false;
            }
        }

        if (!empty()) {
            if (++m_last == m_capacity) {
                m_last = 0;
            }
        }
        m_array[m_last] = elem;
        m_count++;

        return true;
    }

    /**
     * @brief Insert an element to the vector in the specified position.
     * @param[in] position The position in the vector.
     * @param[in] elem The element to add to the vector.
     * @return True on success.
     */
    bool insert(const size_t position, const T &elem)
    {
        AbortIfNot(m_array != nullptr, false);

        if (position > size()) {
            return false;
        }

        if (full()) {
            if (!m_fixedCapacity) {
                AbortIfNot(grow(), false);
            } else {
                return false;
            }
        }

        const size_t offset = (m_first + position) % m_capacity;

        /*
         * Make room for the element.
         */
        if (m_last >= m_first || offset < m_first) {
            /*
             * If the vector does not wrap, or the destination offset only
             * requires to push one end of the array.
             */
            size_t end;
            if (m_last + 1 < m_capacity) {
                end = ++m_last;
            } else {
                /*
                 * Handle the corner case of the last element needing to be
                 * pushed to the front of the array.
                 */
                m_array[0] = m_array[m_last];
                m_last = 0;
                end = m_capacity - 1;
            }
            for (size_t i = end; i >= 1 && i > offset; i--) {
                m_array[i] = m_array[i - 1];
            }
        } else {
            /*
             * If the vector wraps the array and the destination offset requires
             * to push both the beginning and the end of the array.
             */
            m_last++;
            for (size_t i = m_last; i >= 1; i--) {
                m_array[i] = m_array[i - 1];
            }
            m_array[0] = m_array[m_capacity - 1];
            for (size_t i = m_capacity - 1; i > offset; i--) {
                m_array[i] = m_array[i - 1];
            }
        }

        m_array[offset] = elem;
        m_count++;

        return true;
    }

    /**
     * @brief Pull an element from the front of the vector.
     * @param[out] elem On success, set to the first element of the vector.
     * @return True on success.
     */
    bool pop_front(T &elem)
    {
        AbortIfNot(m_array != nullptr, false);

        if (!front(elem)) {
            return false;
        }

        if (++m_first == m_capacity) {
            m_first = 0;
        };
        m_count--;

        return true;
    }

    /**
     * @brief Pull an element from the back of the vector.
     * @param[out] elem On success, set to the last element of the vector.
     * @return True on success.
     */
    bool pop_back(T &elem)
    {
        AbortIfNot(m_array != nullptr, false);

        if (!back(elem)) {
            return false;
        }

        if (m_last) {
            m_last--;
        } else {
            m_last = m_capacity - 1;
        }
        m_count--;

        return true;
    }

    bool erase(const size_t position)
    {
        AbortIfNot(m_array != nullptr, false);

        if (position >= size()) {
            return false;
        }

        //const size_t offset = (m_first + position) % m_capacity;
        // XXX
        m_count--;

        return true;
    }

    // XXX: add iterator

    /**
     * @brief Retrieve the current capacity of the vector.
     * @return The current capacity of the vector.
     */
    size_t capacity() const
    {
        return m_capacity;
    }

    /**
     * @brief Retrieve the number of elements in the vector.
     * @return The number of elements in the vector.
     */
    size_t size() const
    {
        return m_count;
    }

    /**
     * @brief Test whether the vector is empty.
     * @return True if the vector is empty.
     */
    bool empty() const
    {
        return size() == 0;
    }

private:
    /**
     * @brief Grow the capacity of the vector.
     * @return True on success.
     */
    bool grow()
    {
        // TODO
        Assert(false);

        return true;
    }

    /**
     * @brief Test whether the vector has reached capacity.
     * @return True if the vector is full.
     */
    bool full() const
    {
        return size() == capacity();
    }

    /**
     * The underlying storage for the vector.
     */
    T *m_array;

    /**
     * Whether the underlying storage was preallocated outside the container.
     */
    const bool m_unmanagedArray;

    /**
     * The current capacity of the vector.
     */
    size_t m_capacity;

    /**
     * Whether the vector can be dynamically resized when needed.
     */
    const bool m_fixedCapacity;

    /**
     * The number of elements in the vector.
     */
    size_t m_count;

    /**
     * The index of the first element in the storage.
     */
    size_t m_first;

    /**
     * The index of the last element in the storage.
     * Equivalent to (m_first + m_count) % m_capacity.
     */
    size_t m_last;
};

} /* namespace SpherePlusPlus */
