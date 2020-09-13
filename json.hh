/**
 * @file json.hh
 * @author Matthieu Bucchianeri
 * @brief A C++ wrapper of the mjson parser.
 */

#pragma once

#include <math.h>
#include <string.h>
#include <sys/types.h>

#include <sphereplusplus/abort.hh>
#include <sphereplusplus/internal.hh>
#include <sphereplusplus/vector.hh>

namespace SpherePlusPlus {

class JsonString
{
public:
    JsonString() :
        m_string(nullptr),
        m_size(0)
    {
    }

    JsonString(const void *const string, const size_t size) :
        m_string((char *)string),
        m_size(size)
    {
    }

    JsonString(const char *const string) :
        m_string(string),
        m_size(strlen(string))
    {
    }

    bool operator==(const JsonString &other) const
    {
        if (m_size != other.m_size) {
            return false;
        }

        return !strncmp(m_string, other.m_string, m_size);
    }

    bool operator!=(const JsonString &other) const
    {
        return !operator==(other);
    }

    const char *m_string;
    size_t m_size;
};

static inline void __print_values(const JsonString &a, const JsonString &b)
{
    Log_Debug("%.*s, %.*s", a.m_size, a.m_string, b.m_size, b.m_string);
}

/**
 * @brief A JSON document to parse.
 */
class JsonParser
{
public:
    class const_iterator
    {
    public:
        ~const_iterator()
        {
        }

        const_iterator& operator++()
        {
            advance();
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator cit = *this;
            ++*this;
            return cit;
        }

        bool operator!=(const const_iterator &other) const
        {
            return m_state != other.m_state;
        }

        const JsonString operator*()
        {
            return JsonString(m_keyString + 1);
        }

    private:
        class iterator_state
        {
        public:
            iterator_state() :
                m_object(),
                m_keySize(0),
                m_offset(0)
            {
            }

            iterator_state(const JsonString &object, const size_t keySize, const int offset) :
                m_object(object),
                m_keySize(keySize),
                m_offset(offset)
            {
            }

            bool operator!=(const iterator_state &other) const
            {
                return (m_object.m_string != other.m_object.m_string ||
                        m_offset != other.m_offset);
            }

            JsonString m_object;
            size_t m_keySize;
            int m_offset;
        };

        const_iterator(const JsonParser &parser, const bool end = false) :
            m_state(JsonString(parser.m_jsonString, parser.m_jsonSize), 0, 0),
            m_buffer(),
            m_traversal((void *)m_buffer, sizeof(m_buffer)),
            m_keyString(),
            m_keyOffset(),
            m_keySize(),
            m_valueOffset(),
            m_valueSize(),
            m_valueType()
        {
            if (!end) {
                advance();
            }
        }

        void advance()
        {
            /*
             * Advance to the next key.
             */
            int vtype;
            m_state.m_offset =
                mjson_next(m_state.m_object.m_string, m_state.m_object.m_size,
                           m_state.m_offset,
                           &m_keyOffset, &m_keySize,
                           &m_valueOffset, &m_valueSize,
                           &vtype);
            if (m_state.m_offset != 0) {
                /*
                 * Strip the quotes from the key.
                 */
                m_keyOffset += 1;
                m_keySize -= 2;

                /*
                 * Construct the full key path.
                 */
                snprintf(m_keyString + m_state.m_keySize, sizeof (m_keyString) - m_state.m_keySize,
                         ".%.*s", m_keySize, m_state.m_object.m_string + m_keyOffset);

                /*
                 * Recurse into child objects.
                 */
                m_valueType = static_cast<mjson_tok>(vtype);
                if (m_valueType == MJSON_TOK_OBJECT) {
                    m_traversal.push_back(m_state);
                    m_state.m_object = JsonString(m_state.m_object.m_string + m_valueOffset, m_valueSize);
                    m_state.m_keySize += m_keySize + 1;
                    m_state.m_offset = 0;
                    advance();
                }
            } else {
                /*
                 * Return to the parent object after recursion.
                 */
                if (m_traversal.pop_back(m_state) && m_state.m_offset != 0) {
                    advance();
                }
            }
        }

        iterator_state m_state;

        iterator_state m_buffer[10];
        vector<iterator_state> m_traversal;
        char m_keyString[100];

        int m_keyOffset;
        int m_keySize;
        int m_valueOffset;
        int m_valueSize;
        mjson_tok m_valueType;

        friend class JsonParser;
    };

    /**
     * @brief Constructor.
     * @param[in] jsonString A buffer containing a JSON document.
     * @param[in] jsonSize The size of the JSON document, in bytes.
     */
    JsonParser(const void *const jsonString, const size_t jsonSize) :
        m_jsonString((char *)jsonString),
        m_jsonSize(jsonSize)
    {
    }

    /**
     * @brief Constructor.
     * @param[in] jsonString A null-terminated string containing a JSON
     *            document.
     */
    JsonParser(const char *const jsonString) :
        m_jsonString(jsonString),
        m_jsonSize(strlen(jsonString))
    {
    }

    /**
     * @brief Destructor.
     */
    ~JsonParser()
    {
    }

    JsonParser get_object(const char *const key) const
    {
        const char *tok = nullptr;
        int toklen = 0;
        const mjson_tok type = mjson_find(m_jsonString, m_jsonSize, key,
                                          &tok, &toklen);

        if (type == MJSON_TOK_OBJECT) {
            return JsonParser(tok, toklen);
        } else {
            return JsonParser(nullptr, 0);
        }
    }

    static bool get_string(const const_iterator &cit, JsonString &value)
    {
        if (cit.m_valueType == MJSON_TOK_STRING) {
            value = JsonString(cit.m_state.m_object.m_string + cit.m_valueOffset + 1, cit.m_valueSize - 2);
        } else {
            value = JsonString(cit.m_state.m_object.m_string + cit.m_valueOffset, cit.m_valueSize);
        }
        return true;
    }

    static bool get_uint(const const_iterator &cit, unsigned int &value)
    {
        if (cit.m_valueType != MJSON_TOK_NUMBER) {
            return false;
        }

        char buffer[cit.m_valueSize + 1];
        snprintf(buffer, sizeof(buffer), "%.*s",
                 cit.m_valueSize, cit.m_state.m_object.m_string + cit.m_valueOffset);
        char *endptr;
        value = strtoul(buffer, &endptr, 10);
        return !*endptr;
    }

    static bool get_int(const const_iterator &cit, int &value)
    {
        if (cit.m_valueType != MJSON_TOK_NUMBER) {
            return false;
        }

        char buffer[cit.m_valueSize + 1];
        snprintf(buffer, sizeof(buffer), "%.*s",
                 cit.m_valueSize, cit.m_state.m_object.m_string + cit.m_valueOffset);
        char *endptr;
        value = strtol(buffer, &endptr, 10);
        return !*endptr;
    }

    static bool get_bool(const const_iterator &cit, bool &value)
    {
        if (cit.m_valueType == MJSON_TOK_TRUE) {
            value = true;
            return true;
        } else if (cit.m_valueType == MJSON_TOK_FALSE) {
            value = false;
            return true;
        } else {
            return false;
        }
    }

    static bool get_float(const const_iterator &cit, float &value)
    {
        if (cit.m_valueType != MJSON_TOK_NUMBER) {
            return false;
        }

        char buffer[cit.m_valueSize + 1];
        snprintf(buffer, sizeof(buffer), "%.*s",
                 cit.m_valueSize, cit.m_state.m_object.m_string + cit.m_valueOffset);
        char *endptr;
        value = strtof(buffer, &endptr);
        return !*endptr;
    }

    const_iterator cbegin() const
    {
        return const_iterator(*this);
    }

    const_iterator cend() const
    {
        return const_iterator(*this, true);
    }

private:
    /**
     * The buffer containing the JSON document.
     */
    const char *const m_jsonString;

    /**
     * The size of the JSON document, in bytes.
     */
    const size_t m_jsonSize;
};

} /* namespace SpherePlusPlus */
