// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_fixed_size_map.hpp
///
//-----------------------------------------------------------------------------
#ifndef CONDUIT_FIXED_SIZE_MAP_HPP
#define CONDUIT_FIXED_SIZE_MAP_HPP
#include "conduit_error.hpp"

#include <array>

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

/*!
 * @brief This class represents a fixed-sized map where keys and values are
 *        stored in contiguous arrays.
 *
 * @tparam KeyType The type use for keys.
 * @tparam ValueType The type use for values.
 * @tparam N The max number of elements.
 */
template <typename KeyType, typename ValueType, int N>
class fixed_size_map
{
public:
    int size() const { return m_length; }
    const ValueType &get(int index) const { return m_values[index]; }
    ValueType &operator[](const KeyType &key)
    {
        ValueType *v = find(key);
        if(v == nullptr)
        {
            if(m_length < N)
            {
                m_keys[m_length] = key;
                m_values[m_length] = ValueType {};
                v = m_values.data() + m_length;
                m_length++;
            }
            else
            {
                CONDUIT_ERROR("Out of space.");
            }
        }
        return *v;
    }
    const ValueType &operator[](const KeyType &key) const
    {
        ValueType *v = find(key);
        if(v == nullptr)
        {
            CONDUIT_ERROR("Key not found.");
        }
        return *v;
    }

private:
    /*!
     * @brief Find the value for the given key.
     *
     * @param k The search key.
     *
     * @return A pointer to the value for the key or nullptr if the key was not found.
     *
     * @note These maps are assumed to be small so keys are searched in order.
     */
    ValueType *find(const KeyType &k) const
    {
        for(int i = 0; i < m_length; i++)
        {
            if(m_keys[i] == k)
            {
                return const_cast<ValueType *>(m_values.data()) + i;
            }
        }
        return nullptr;
    }

    std::array<KeyType, N> m_keys {};
    std::array<ValueType, N> m_values {};
    int m_length {0};
};

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
